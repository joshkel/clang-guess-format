#include <iostream>

using namespace std;

#include "clang/Format/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/YAMLTraits.h"

using std::cout;
using namespace llvm;
using namespace clang;
using namespace clang::format;
using clang::tooling::Replacements;

struct CodeFile {
  CodeFile(std::unique_ptr<const llvm::MemoryBuffer> Code, std::string FileName)
    : Code(std::move(Code)),
      FileName(FileName),
      Ranges { tooling::Range(0, this->Code->getBufferSize()) }
  {
  }

  std::unique_ptr<const llvm::MemoryBuffer> Code;
  std::string FileName;
  std::vector<tooling::Range> Ranges;
};

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

// Source: https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C
int levenshtein(const char *s1, const char *s2,
    const unsigned int s1len, const unsigned int s2len)
{
  unsigned int column[s1len+1];
  for (unsigned int y = 1; y <= s1len; y++) {
    column[y] = y;
  }
  for (unsigned int x = 1; x <= s2len; x++) {
    column[0] = x;
    for (unsigned int y = 1, lastdiag = x - 1; y <= s1len; y++) {
      unsigned int olddiag = column[y];
      column[y] = MIN3(column[y] + 1, column[y - 1] + 1,
          lastdiag + (s1[y - 1] == s2[x - 1] ? 0 : 1));
      lastdiag = olddiag;
    }
  }
  return column[s1len];
}

int getTotalDistance(const llvm::MemoryBuffer& Code,
                     const Replacements& FormatChanges)
{
  int TotalDistance = 0;
  for (const auto& i : FormatChanges) {
    TotalDistance += levenshtein(
        Code.getBufferStart() + i.getOffset(), i.getReplacementText().data(),
        i.getLength(), i.getReplacementText().size());
  }
  return TotalDistance;
}

template<typename T>
std::vector<T> getValues()
{
  static_assert(sizeof(T) == -1, "getValues must be specialized");
}

template<>
std::vector<bool> getValues<bool>()
{
  return { false, true };
}

template<typename T>
std::string stringize(T Value)
{
  std::string Text;
  llvm::raw_string_ostream Stream(Text);
  llvm::yaml::Output Output(Stream);
  yamlize(Output, Value, true);
  return Text;
}

template<typename T>
void tryFormat(FormatStyle& Style, const std::vector<CodeFile>& CodeFiles,
               const char *ValueName, const std::vector<T>& Values,
               std::function<void(FormatStyle&, T)> Apply)
{
  bool FirstRun = true;
  bool HasBestValue = false;
  int MinTotalDistance = std::numeric_limits<int>::max();
  T BestValue = {};

  for (const T& v : Values) {
    Apply(Style, v);

    if (FirstRun) {
      FirstRun = false;
      outs() << "# ";
    } else {
      outs() << ", ";
    }
    outs() << stringize(v) << " ";

    int TotalDistance = 0;
    bool HasFailures = false;
    for (const auto& CodeFile : CodeFiles) {
      bool IncompleteFormat;
      Replacements FormatChanges = reformat(Style, CodeFile.Code->getBuffer(),
                                            CodeFile.Ranges, CodeFile.FileName,
                                            &IncompleteFormat);
      if (IncompleteFormat) {
        errs() << CodeFile.FileName << ": " << ValueName << " " << v << "Failed\n";
        HasFailures = true;
        continue;
      }

      TotalDistance += getTotalDistance(*CodeFile.Code, FormatChanges);
    }

    if (HasFailures) {
      outs() << "failed";
      continue;
    }

    if (!HasBestValue) {
      MinTotalDistance = TotalDistance;
      BestValue = v;
      HasBestValue = true;
    } else {
      if (TotalDistance < MinTotalDistance) {
        MinTotalDistance = TotalDistance;
        BestValue = v;
      }
    }
    outs() << TotalDistance;
  }

  outs() << "\n" << ValueName << ": " << stringize(BestValue) << "\n";
  Apply(Style, BestValue);
}

template<typename T>
void tryFormat(FormatStyle& Style, const std::vector<CodeFile>& CodeFiles,
               const char *ValueName, T FormatStyle::*Value)
{
  tryFormat<T>(Style, CodeFiles, ValueName, getValues<T>(),
               [Value](FormatStyle& Style, T v) { (Style.*Value) = v; });
}

#define TRY_FORMAT(Style, CodeFiles, Value) tryFormat(Style, CodeFiles, #Value, &FormatStyle::Value)

int main(int argc, char **argv)
{
  std::vector<CodeFile> CodeFiles;
  for (int i = 1; i < argc; i++) {
    const std::string FileName = argv[i];
    ErrorOr<std::unique_ptr<MemoryBuffer>> CodeOrErr =
        MemoryBuffer::getFileOrSTDIN(FileName);
    if (std::error_code EC = CodeOrErr.getError()) {
      errs() << "Failed to read " << FileName << ": " << EC.message() << "\n";
      return 1;
    }
    std::unique_ptr<const llvm::MemoryBuffer> Code = std::move(CodeOrErr.get());
    if (Code->getBufferSize() == 0) {
      // Empty files are formatted correctly.
      errs() << FileName << " is empty, skipping.\n";
      continue;
    }
    CodeFiles.push_back({ std::move(Code), FileName });
  }

  outs() << "# Total edit distance (lower is better)\n";

  FormatStyle Style;

  tryFormat<std::string>(
      Style, CodeFiles, "BasedOnStyle",
      { "LLVM", "Google", "Chromium", "Mozilla", "WebKit" },
      [](FormatStyle& Style, std::string StyleName) {
        if (!clang::format::getPredefinedStyle(StyleName, FormatStyle::LK_Cpp, &Style)) {
          errs() << "Failed to get style " << StyleName << "\n";
          exit(1);
        }
      });

  TRY_FORMAT(Style, CodeFiles, AllowShortIfStatementsOnASingleLine);

  /*
  FormatStyle Style;
  clang::format::getPredefinedStyle("Mozilla", FormatStyle::LK_Cpp, &Style);
  for (int i = 0; i <= 1; i++) {
    Style.AllowShortIfStatementsOnASingleLine = i;

    bool IncompleteFormat;
    Replacements FormatChanges = reformat(Style, Code->getBuffer(), Ranges, FileName, &IncompleteFormat);
    if (IncompleteFormat) {
      cout << "Failed\n";
      return 1;
    }

    cout << i << ": " << FormatChanges.size() << endl;

    int TotalDistance = getTotalDistance(*Code, FormatChanges);
    cout << "    " << TotalDistance << endl;
  }
  */
}
