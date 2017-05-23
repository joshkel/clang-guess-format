#include <iostream>

using namespace std;

#include "clang/Format/Format.h"
#include "llvm/Support/MemoryBuffer.h"

using std::cout;
using namespace llvm;
using namespace clang;
using namespace clang::format;
using clang::tooling::Replacements;

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

// Source: https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C
int levenshtein(const char *s1, const char *s2, const unsigned int s1len, const unsigned int s2len) {
    unsigned int x, y, lastdiag, olddiag;
    unsigned int column[s1len+1];
    for (y = 1; y <= s1len; y++)
        column[y] = y;
    for (x = 1; x <= s2len; x++) {
        column[0] = x;
        for (y = 1, lastdiag = x-1; y <= s1len; y++) {
            olddiag = column[y];
            column[y] = MIN3(column[y] + 1, column[y-1] + 1, lastdiag + (s1[y-1] == s2[x-1] ? 0 : 1));
            lastdiag = olddiag;
        }
    }
    return(column[s1len]);
}

int main(int argc, char **argv)
{
  const std::string FileName(argv[1]);
  const char * const try_formats[] = { "LLVM", "Google", "Chromium", "Mozilla", "WebKit" };

  for (const char * format : try_formats) {
    ErrorOr<std::unique_ptr<MemoryBuffer>> CodeOrErr = MemoryBuffer::getFileOrSTDIN(FileName);
    if (std::error_code EC = CodeOrErr.getError()) {
      errs() << EC.message() << "\n";
      return 1;
    }
    std::unique_ptr<llvm::MemoryBuffer> Code = std::move(CodeOrErr.get());
    if (Code->getBufferSize() == 0)
      return false; // Empty files are formatted correctly.
    std::vector<tooling::Range> Ranges = { tooling::Range(0, Code->getBufferSize()) };
    FormatStyle Style;
    if (!clang::format::getPredefinedStyle(format, FormatStyle::LK_Cpp, &Style)) {
      cout << "Failed to get style " << format << "\n";
      continue;
    }
    bool IncompleteFormat;
    Replacements FormatChanges = reformat(Style, Code->getBuffer(), Ranges, FileName, &IncompleteFormat);
    if (IncompleteFormat) {
      cout << "Failed\n";
      return 1;
    }

    cout << format << ": " << FormatChanges.size() << endl;

    int TotalDistance = 0;
    for (const auto& i : FormatChanges) {
      TotalDistance += levenshtein(Code->getBufferStart() + i.getOffset(), i.getReplacementText().data(), i.getLength(), i.getReplacementText().size());
    }
    cout << "    " << TotalDistance << endl;
  }
}
