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

typedef FormatStyle FS;

template<>
std::vector<FS::BracketAlignmentStyle> getValues<FS::BracketAlignmentStyle>()
{
  return { FS::BAS_Align, FS::BAS_DontAlign, FS::BAS_AlwaysBreak };
}

template<>
std::vector<FS::ShortFunctionStyle> getValues<FS::ShortFunctionStyle>()
{
  return { FS::SFS_None, FS::SFS_Empty, FS::SFS_Inline, FS::SFS_All };
}

template<typename T>
struct MemberSetter {
  T FormatStyle::*Member;
  void operator()(FormatStyle& Style, T Value) {
    Style.*Member = Value;
  }
};

template<typename T>
MemberSetter<T> memberSetter(T FormatStyle::*Member) {
  return { Member };
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
  // Total edit distance for each value
  std::vector<std::pair<T, int>> Results;
  const int FAILED = std::numeric_limits<int>::max();

  for (const T& v : Values) {
    Apply(Style, v);

    int TotalDistance = 0;
    bool HasFailures = false;
    for (const auto& CodeFile : CodeFiles) {
      bool IncompleteFormat = false;
      Replacements FormatChanges = reformat(Style, CodeFile.Code->getBuffer(),
                                            CodeFile.Ranges, CodeFile.FileName,
                                            &IncompleteFormat);
      if (IncompleteFormat) {
        errs() << CodeFile.FileName << ": " << ValueName << " " << v << " Failed\n";
        HasFailures = true;
        break;
      }

      TotalDistance += getTotalDistance(*CodeFile.Code, FormatChanges);
    }

    if (HasFailures)
      Results.push_back({v, FAILED});
    else
      Results.push_back({v, TotalDistance});
  }

  std::sort(Results.begin(), Results.end(),
      [](const std::pair<T, int>& a, const std::pair<T, int>& b) {
        return a.second < b.second;
      });

  if (Results.empty()) {
    throw std::runtime_error(std::string("Failed to find any values for ")
                             + ValueName);
  } else if (Results[0].second == FAILED) {
    throw std::runtime_error(std::string("No usable values found for ")
                             + ValueName);
  }

  outs() << "\n";

  // Print the best value as a configuration setting.
  if (Results.size() > 1 && Results[0].second == Results[1].second)
    outs() << "# " << ValueName << ": ???\n";
  else
    outs() << ValueName << ": " << stringize(Results[0].first) << "\n";

  // Print full results as a comment.
  outs() << "# ";
  for (const auto& i : Results) {
    if (i != *Results.begin())
      outs() << ", ";
    outs() << stringize(i.first) << " ";
    if (i.second == FAILED)
      outs() << "failed";
    else
      outs() << i.second;
  }
  outs() << "\n";

  // And lock in one of the values for further work.
  Apply(Style, Results[0].first);
}

template<typename T>
void tryFormat(FormatStyle& Style, const std::vector<CodeFile>& CodeFiles,
               const char *ValueName, T FormatStyle::*Member)
{
  tryFormat<T>(Style, CodeFiles, ValueName, getValues<T>(),
               memberSetter(Member));
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

  try {
    tryFormat<std::string>(
        Style, CodeFiles, "BasedOnStyle",
        { "LLVM", "Google", "Chromium", "Mozilla", "WebKit" },
        [](FormatStyle& Style, std::string StyleName) {
          if (!clang::format::getPredefinedStyle(StyleName, FormatStyle::LK_Cpp, &Style)) {
            errs() << "Failed to get style " << StyleName << "\n";
            exit(1);
          }
        });

    tryFormat<int>(Style, CodeFiles, "AccessModifierOffset",
                   { -8, -4, -2, 0, 2, 4, 8 },
                   memberSetter(&FormatStyle::AccessModifierOffset));

    //TRY_FORMAT(Style, CodeFiles, AlignAfterOpenBracket);
    TRY_FORMAT(Style, CodeFiles, AlignConsecutiveAssignments);
    TRY_FORMAT(Style, CodeFiles, AlignConsecutiveDeclarations);
    TRY_FORMAT(Style, CodeFiles, AlignEscapedNewlinesLeft);
    TRY_FORMAT(Style, CodeFiles, AlignOperands);
    TRY_FORMAT(Style, CodeFiles, AlignTrailingComments);
    TRY_FORMAT(Style, CodeFiles, AllowAllParametersOfDeclarationOnNextLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortBlocksOnASingleLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortCaseLabelsOnASingleLine);
    //TRY_FORMAT(Style, CodeFiles, AllowShortFunctionsOnASingleLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortIfStatementsOnASingleLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortLoopsOnASingleLine);
  } catch (std::exception& e) {
    errs() << e.what() << "\n";
    return 1;
  }

  return 0;
}
