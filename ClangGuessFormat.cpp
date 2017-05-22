#include <iostream>

using namespace std;

#include "clang/Format/Format.h"
#include "llvm/Support/MemoryBuffer.h"

using std::cout;
using namespace llvm;
using namespace clang;
using namespace clang::format;
using clang::tooling::Replacements;

int main(int argc, char **argv)
{
  const std::string FileName(argv[1]);
  const char * const try_formats[] = { "LLVM", "Google", "Chromium", "Mozilla" };

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
  }
}
