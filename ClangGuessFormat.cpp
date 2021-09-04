#include <iostream>

using namespace std;

#include "clang/Format/Format.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/YAMLTraits.h"

using std::cout;
using namespace llvm;
using namespace clang;
using namespace clang::format;
using clang::tooling::Replacements;
using llvm::Optional;

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
std::vector<FS::AlignConsecutiveStyle> getValues<FS::AlignConsecutiveStyle>()
{
  return { FS::ACS_None, FS::ACS_Consecutive, FS::ACS_AcrossEmptyLines,
           FS::ACS_AcrossEmptyLines, FS::ACS_AcrossEmptyLinesAndComments };
}

template<>
std::vector<FS::BinaryOperatorStyle> getValues<FS::BinaryOperatorStyle>()
{
  return { FS::BOS_None, FS::BOS_NonAssignment, FS::BOS_All };
}

template<>
std::vector<FS::BinPackStyle> getValues<FS::BinPackStyle>()
{
  return { FS::BPS_Auto, FS::BPS_Always, FS::BPS_Never };
}

template<>
std::vector<FS::BitFieldColonSpacingStyle> getValues<FS::BitFieldColonSpacingStyle>()
{
  return { FS::BFCS_Both, FS::BFCS_None, FS::BFCS_Before, FS::BFCS_After };
}

template<>
std::vector<FS::BraceBreakingStyle> getValues<FS::BraceBreakingStyle>()
{
  // BS_Custom is not yet supported.  (See "BraceWrapping" below.)
  return { FS::BS_Attach, FS::BS_Linux, FS::BS_Mozilla, FS::BS_Stroustrup,
           FS::BS_Allman, FS::BS_Whitesmiths, FS::BS_GNU, FS::BS_WebKit };
}

template<>
std::vector<FS::BraceWrappingAfterControlStatementStyle> getValues<FS::BraceWrappingAfterControlStatementStyle>()
{
  return { FS::BWACS_Never, FS::BWACS_MultiLine, FS::BWACS_Always };
}

template<>
std::vector<FS::BracketAlignmentStyle> getValues<FS::BracketAlignmentStyle>()
{
  return { FS::BAS_Align, FS::BAS_DontAlign, FS::BAS_AlwaysBreak };
}

template<>
std::vector<FS::BreakConstructorInitializersStyle> getValues<FS::BreakConstructorInitializersStyle>()
{
  return { FS::BCIS_BeforeColon, FS::BCIS_BeforeComma, FS::BCIS_AfterColon };
}

template<>
std::vector<FS::BreakInheritanceListStyle> getValues<FS::BreakInheritanceListStyle>()
{
  return { FS::BILS_BeforeColon, FS::BILS_BeforeComma, FS::BILS_AfterColon };
}

template<>
std::vector<FS::BreakTemplateDeclarationsStyle> getValues<FS::BreakTemplateDeclarationsStyle>()
{
  return { FS::BTDS_No, FS::BTDS_MultiLine, FS::BTDS_Yes };
}

template<>
std::vector<FS::DefinitionReturnTypeBreakingStyle> getValues<FS::DefinitionReturnTypeBreakingStyle>()
{
  return { FS::DRTBS_None, FS::DRTBS_All, FS::DRTBS_TopLevel };
}

template<>
std::vector<FS::EmptyLineBeforeAccessModifierStyle> getValues<FS::EmptyLineBeforeAccessModifierStyle>()
{
  return { FS::ELBAMS_Never, FS::ELBAMS_Leave, FS::ELBAMS_LogicalBlock, FS::ELBAMS_Always };
}

template<>
std::vector<FS::EscapedNewlineAlignmentStyle> getValues<FS::EscapedNewlineAlignmentStyle>()
{
  return { FS::ENAS_DontAlign, FS::ENAS_Left, FS::ENAS_Right };
}

template<>
std::vector<FS::IndentExternBlockStyle> getValues<FS::IndentExternBlockStyle>()
{
  return { FS::IEBS_AfterExternBlock, FS::IEBS_NoIndent, FS::IEBS_Indent };
}

template<>
std::vector<FS::LanguageStandard> getValues<FS::LanguageStandard>()
{
  return { FS::LS_Cpp03, FS::LS_Cpp11, FS::LS_Auto };
}

template<>
std::vector<FS::NamespaceIndentationKind> getValues<FS::NamespaceIndentationKind>()
{
  return { FS::NI_None, FS::NI_Inner, FS::NI_All };
}

template<>
std::vector<FS::OperandAlignmentStyle> getValues<FS::OperandAlignmentStyle>()
{
  return { FS::OAS_DontAlign, FS::OAS_Align, FS::OAS_AlignAfterOperator };
}

template<>
std::vector<FS::PointerAlignmentStyle> getValues<FS::PointerAlignmentStyle>()
{
  return { FS::PAS_Left, FS::PAS_Right, FS::PAS_Middle };
}

template<>
std::vector<FS::PPDirectiveIndentStyle> getValues<FS::PPDirectiveIndentStyle>()
{
  return { FS::PPDIS_None, FS::PPDIS_AfterHash, FS::PPDIS_BeforeHash };
}

template<>
std::vector<FS::ReturnTypeBreakingStyle> getValues<FS::ReturnTypeBreakingStyle>()
{
  return { FS::RTBS_None, FS::RTBS_All, FS::RTBS_TopLevel,
           FS::RTBS_AllDefinitions, FS::RTBS_TopLevelDefinitions };
}

template<>
std::vector<FS::ShortBlockStyle> getValues<FS::ShortBlockStyle>()
{
  return { FS::SBS_Never, FS::SBS_Empty, FS::SBS_Always };
}

template<>
std::vector<FS::ShortFunctionStyle> getValues<FS::ShortFunctionStyle>()
{
  return { FS::SFS_None, FS::SFS_InlineOnly, FS::SFS_Empty, FS::SFS_Inline,
           FS::SFS_All };
}

template<>
std::vector<FS::ShortIfStyle> getValues<FS::ShortIfStyle>()
{
  return { FS::SIS_Never, FS::SIS_WithoutElse, FS::SIS_Always };
}

template<>
std::vector<FS::ShortLambdaStyle> getValues<FS::ShortLambdaStyle>()
{
  return { FS::SLS_None, FS::SLS_Empty, FS::SLS_Inline, FS::SLS_All };
}

template<>
std::vector<FS::SpaceAroundPointerQualifiersStyle> getValues<FS::SpaceAroundPointerQualifiersStyle>()
{
  return { FS::SAPQ_Default, FS::SAPQ_Before, FS::SAPQ_After, FS::SAPQ_Both };
}

template<>
std::vector<FS::SpaceBeforeParensOptions> getValues<FS::SpaceBeforeParensOptions>()
{
  return { FS::SBPO_Never, FS::SBPO_ControlStatements, FS::SBPO_Always };
}

template<>
std::vector<FS::TrailingCommaStyle> getValues<FS::TrailingCommaStyle>()
{
  return { FS::TCS_None, FS::TCS_Wrapped };
}

template<>
std::vector<FS::UseTabStyle> getValues<FS::UseTabStyle>()
{
  return { FS::UT_Never, FS::UT_ForIndentation, FS::UT_ForContinuationAndIndentation,
           FS::UT_AlignWithSpaces, FS::UT_Always };
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
  llvm::yaml::EmptyContext ctx;
  yamlize(Output, Value, true, ctx);
  return Text;
}

template<typename T>
typename std::enable_if<!llvm::yaml::has_ScalarTraits<T>::value, std::string>::type
valueToString(const FormatStyle& Style, const char *ValueName, T Value)
{
  // Hack: We have no generic way of converting a single value to a string.
  // (Within Format.cpp, ScalarEnumerationTraits is specialized for each enum,
  // but that's not accessible outside of the file.)  The only generic solution
  // is to dump the entire configuration to string then extract what we want.
  const std::string Config = clang::format::configurationAsText(Style);
  std::string ValueKey = "\n" + std::string(ValueName) + ": ";
  std::string::size_type Found = Config.find(ValueKey);
  if (Found == std::string::npos)
    throw std::runtime_error(std::string("Failed to find ") + ValueName
                             + " within config");
  std::string::size_type From = Found + ValueKey.size();
  std::string::size_type To = Config.find('\n', From);
  return StringRef(Config.c_str() + From, To - From).trim().str();
}

template<typename T>
typename std::enable_if<llvm::yaml::has_ScalarTraits<T>::value, std::string>::type
valueToString(const FormatStyle& Style, const char *ValueName, T Value)
{
  return stringize(Value);
}

template<typename T>
void tryFormat(FormatStyle& Style, const std::vector<CodeFile>& CodeFiles,
               const char *ValueName, const std::vector<T>& Values,
               std::function<void(FormatStyle&, T)> Apply,
               const T &Default,
               Optional<T> Preferred = llvm::None)
{
  // Total edit distance for each value
  struct ResultType {
    T Value;
    std::string ValueString;
    int TotalDistance;
  };
  std::vector<ResultType> Results;
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

    std::string ValueString = valueToString(Style, ValueName, v);
    if (HasFailures) {
      Results.push_back({v, ValueString, FAILED});
    }
    else
      Results.push_back({v, ValueString, TotalDistance});
  }

  std::sort(Results.begin(), Results.end(),
      [](const ResultType& a, const ResultType& b) {
        return a.TotalDistance < b.TotalDistance;
      });

  if (Results.empty()) {
    throw std::runtime_error(std::string("Failed to find any values for ")
                             + ValueName);
  } else if (Results[0].TotalDistance == FAILED) {
    throw std::runtime_error(std::string("No usable values found for ")
                             + ValueName);
  }

  outs() << "\n";

  // Print the best value as a configuration setting.  Allow the use of
  // "preferred" values - e.g., if ReflowComments true and false give the same
  // results, it must be because comments were already reflowed.
  Optional<std::string> BestValueString;
  T *BestValue = nullptr;
  if (Results.size() == 1 || Results[0].TotalDistance < Results[1].TotalDistance) {
    BestValueString = Results[0].ValueString;
    BestValue = &Results[0].Value;
  }
  for (size_t i = 0; i < Results.size(); i++) {
    if (Preferred && Results[i].Value == *Preferred) {
      BestValueString = Results[i].ValueString;
      BestValue = &Results[i].Value;
      break;
    } else if (Results[i].Value == Default) {
      BestValueString = Results[i].ValueString;
      BestValue = &Results[i].Value;
    } else if (i + 1 < Results.size() && Results[i].TotalDistance != Results[i + 1].TotalDistance) {
      break;
    }
  }
  if (BestValueString) {
    if (*BestValue == Default)
      outs() << "# " << ValueName << ": " << *BestValueString << "\n";
    else
      outs() << ValueName << ": " << *BestValueString << "\n";
  } else {
    outs() << "# " << ValueName << ": ???\n";
  }

  // Print full results as a comment.
  outs() << "# ";
  for (const auto& i : Results) {
    if (i.Value != Results[0].Value)
      outs() << ", ";
    if (i.Value == Default)
      outs() << i.ValueString << "* ";
    else
      outs() << i.ValueString << " ";
    if (i.TotalDistance == FAILED)
      outs() << "failed";
    else
      outs() << i.TotalDistance;
  }
  outs() << "\n";

  // And lock in one of the values for further work.
  if (BestValue)
    Apply(Style, *BestValue);
  else
    Apply(Style, Results[0].Value);
}

template<typename T>
void tryFormat(FormatStyle& Style, const std::vector<CodeFile>& CodeFiles,
               const char *ValueName, T FormatStyle::*Member, const T &Default,
               Optional<T> Preferred = llvm::None)
{
  tryFormat<T>(Style, CodeFiles, ValueName, getValues<T>(),
               memberSetter(Member), Default, Preferred);
}

#define TRY_FORMAT(Style, CodeFiles, Value, ...) \
    tryFormat(Style, CodeFiles, #Value, &FormatStyle::Value, Style.Value, ##__VA_ARGS__)

void writeUnguessableSetting(const char *ValueName)
{
  outs() << "\n# " << ValueName << ": ???\n";
  outs() << "# (auto detection is not supported; please manually set)\n";
}

void writeNotApplicableSetting(const char *ValueName)
{
  outs() << "\n# " << ValueName << " does not apply to C/C++ code\n";
}

void writeAdvancedSetting(const char *ValueName)
{
  outs() << "\n# " << ValueName << " is an advanced setting and is omitted\n";
}

int main(int argc, char **argv)
{
  std::vector<CodeFile> CodeFiles;
  int ColumnLimitArg = -1;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--column-limit") {
      i++;
      if (i >= argc) {
        errs() << "--column-limit is missing a value\n";
        return 1;
      }
      ColumnLimitArg = std::atoi(argv[i]);
      continue;
    }
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
    CodeFiles.emplace_back( std::move(Code), FileName );
  }

  outs() << "# Total edit distance (lower is better)\n";
  outs() << "# Default values are marked with an asterisk\n";

  FormatStyle Style;

  try {

    tryFormat<std::string>(
        Style, CodeFiles, "BasedOnStyle",
        { "LLVM", "Google", "Chromium", "Mozilla", "WebKit" },
        [ColumnLimitArg](FormatStyle& Style, std::string StyleName) {
          if (!clang::format::getPredefinedStyle(StyleName, FormatStyle::LK_Cpp, &Style)) {
            errs() << "Failed to get style " << StyleName << "\n";
            exit(1);
          }
          if (ColumnLimitArg >= 0)
            Style.ColumnLimit = ColumnLimitArg;
        }, "LLVM");

    if (ColumnLimitArg >= 0)
      outs() << "\nColumnLimit: " << ColumnLimitArg << "\n";
    else
      outs() << "\n# ColumnLimit: " << Style.ColumnLimit << "\n";

    TRY_FORMAT(Style, CodeFiles, UseTab);

    tryFormat<int>(Style, CodeFiles, "TabWidth",
                   { 1, 2, 3, 4, 8 },
                   memberSetter(&FormatStyle::TabWidth),
                   Style.TabWidth);

    tryFormat<int>(Style, CodeFiles, "IndentWidth",
                   { 1, 2, 3, 4, 8 },
                   memberSetter(&FormatStyle::IndentWidth),
                   Style.IndentWidth);

    tryFormat<int>(Style, CodeFiles, "ContinuationIndentWidth",
                   { 1, 2, 3, 4, 8 },
                   memberSetter(&FormatStyle::ContinuationIndentWidth),
                   Style.ContinuationIndentWidth);

    tryFormat<int>(Style, CodeFiles, "AccessModifierOffset",
                   { -8, -4, -2, -1, 0, 1, 2, 4, 8 },
                   memberSetter(&FormatStyle::AccessModifierOffset),
                   Style.AccessModifierOffset);

    tryFormat<int>(Style, CodeFiles, "ConstructorInitializerIndentWidth",
                   { 0, 1, 2, 3, 4, 8 },
                   memberSetter(&FormatStyle::AccessModifierOffset),
                   Style.AccessModifierOffset);

    TRY_FORMAT(Style, CodeFiles, AlignAfterOpenBracket);
    TRY_FORMAT(Style, CodeFiles, AlignConsecutiveAssignments);
    TRY_FORMAT(Style, CodeFiles, AlignConsecutiveBitFields);
    TRY_FORMAT(Style, CodeFiles, AlignConsecutiveDeclarations);
    TRY_FORMAT(Style, CodeFiles, AlignConsecutiveMacros);
    TRY_FORMAT(Style, CodeFiles, AlignEscapedNewlines);
    TRY_FORMAT(Style, CodeFiles, AlignOperands);
    TRY_FORMAT(Style, CodeFiles, AlignTrailingComments);
    TRY_FORMAT(Style, CodeFiles, AllowAllArgumentsOnNextLine);
    TRY_FORMAT(Style, CodeFiles, AllowAllConstructorInitializersOnNextLine);
    TRY_FORMAT(Style, CodeFiles, AllowAllParametersOfDeclarationOnNextLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortBlocksOnASingleLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortCaseLabelsOnASingleLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortEnumsOnASingleLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortFunctionsOnASingleLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortIfStatementsOnASingleLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortLambdasOnASingleLine);
    TRY_FORMAT(Style, CodeFiles, AllowShortLoopsOnASingleLine);

    if (Style.AlwaysBreakAfterDefinitionReturnType != FormatStyle::DRTBS_None) {
      outs() << "\nAlwaysBreakAfterDefinitionReturnType: None\n";
      outs() << "# Deprecated; replaced with AlwaysBreakAfterReturnType\n";
    }

    TRY_FORMAT(Style, CodeFiles, AlwaysBreakAfterReturnType);
    TRY_FORMAT(Style, CodeFiles, AlwaysBreakBeforeMultilineStrings);
    TRY_FORMAT(Style, CodeFiles, AlwaysBreakTemplateDeclarations);
    TRY_FORMAT(Style, CodeFiles, BinPackArguments);
    TRY_FORMAT(Style, CodeFiles, BinPackParameters);
    TRY_FORMAT(Style, CodeFiles, BitFieldColonSpacing);
    writeAdvancedSetting("BraceWrapping");
    writeNotApplicableSetting("BreakAfterJavaFieldAnnotations");
    TRY_FORMAT(Style, CodeFiles, BreakBeforeBinaryOperators);
    TRY_FORMAT(Style, CodeFiles, BreakBeforeBraces);
    TRY_FORMAT(Style, CodeFiles, BreakBeforeConceptDeclarations);
    TRY_FORMAT(Style, CodeFiles, BreakBeforeTernaryOperators);
    TRY_FORMAT(Style, CodeFiles, BreakConstructorInitializers);
    TRY_FORMAT(Style, CodeFiles, BreakInheritanceList);
    TRY_FORMAT(Style, CodeFiles, BreakStringLiterals);

    // TODO: Could search code for, e.g., `NOLINT:.*` or `LCOV_EXCL` or `^ IWYU pragma:`
    writeUnguessableSetting("CommentPragmas");

    TRY_FORMAT(Style, CodeFiles, CompactNamespaces);
    TRY_FORMAT(Style, CodeFiles, ConstructorInitializerAllOnOneLineOrOnePerLine);
    TRY_FORMAT(Style, CodeFiles, Cpp11BracedListStyle);
    writeUnguessableSetting("UseCRLF");
    writeUnguessableSetting("DeriveLineEnding");
    writeNotApplicableSetting("DisableFormat");
    TRY_FORMAT(Style, CodeFiles, EmptyLineBeforeAccessModifier);
    TRY_FORMAT(Style, CodeFiles, ExperimentalAutoDetectBinPacking);
    writeUnguessableSetting("ForEachMacros");
    TRY_FORMAT(Style, CodeFiles, FixNamespaceComments);
    writeUnguessableSetting("IncludeCategories");
    writeUnguessableSetting("IncludeIsMainRegex");
    writeAdvancedSetting("IncludeStyle");
    TRY_FORMAT(Style, CodeFiles, IndentCaseBlocks);
    TRY_FORMAT(Style, CodeFiles, IndentCaseLabels);
    TRY_FORMAT(Style, CodeFiles, IndentExternBlock);
    TRY_FORMAT(Style, CodeFiles, IndentGotoLabels);
    TRY_FORMAT(Style, CodeFiles, IndentPPDirectives);
    TRY_FORMAT(Style, CodeFiles, IndentRequires);
    TRY_FORMAT(Style, CodeFiles, IndentWrappedFunctionNames);
    TRY_FORMAT(Style, CodeFiles, InsertTrailingCommas);
    writeNotApplicableSetting("JavaImportGroups");
    writeNotApplicableSetting("JavaScriptQuotes");
    writeNotApplicableSetting("JavaScriptWrapImports");
    TRY_FORMAT(Style, CodeFiles, KeepEmptyLinesAtTheStartOfBlocks);
    writeNotApplicableSetting("Language");
    writeUnguessableSetting("MacroBlockBegin");
    writeUnguessableSetting("MacroBlockEnd");

    tryFormat<int>(Style, CodeFiles, "MaxEmptyLinesToKeep",
                   { 0, 1, 2, 3, 4, 1000 },
                   memberSetter(&FormatStyle::MaxEmptyLinesToKeep),
                   Style.MaxEmptyLinesToKeep, 2);

    TRY_FORMAT(Style, CodeFiles, NamespaceIndentation);
    writeUnguessableSetting("NamespaceMacros");
    writeNotApplicableSetting("ObjCBinPackProtocolList");
    writeNotApplicableSetting("ObjCBlockIndentWidth");
    writeNotApplicableSetting("ObjCBreakBeforeNestedBlockParam");
    writeNotApplicableSetting("ObjCSpaceAfterProperty");
    writeNotApplicableSetting("ObjCSpaceBeforeProtocolList");
    writeAdvancedSetting("PenaltyBreakAssignment");
    writeAdvancedSetting("PenaltyBreakBeforeFirstCallParameter");
    writeAdvancedSetting("PenaltyBreakComment");
    writeAdvancedSetting("PenaltyBreakFirstLessLess");
    writeAdvancedSetting("PenaltyBreakString");
    writeAdvancedSetting("PenaltyBreakTemplateDeclaration");
    writeAdvancedSetting("PenaltyExcessCharacter");
    writeAdvancedSetting("PenaltyIndentedWhitespace");
    writeAdvancedSetting("PenaltyReturnTypeOnItsOwnLine");
    TRY_FORMAT(Style, CodeFiles, PointerAlignment);
    TRY_FORMAT(Style, CodeFiles, DerivePointerAlignment, { false });
    writeAdvancedSetting("RawStringFormat");
    writeAdvancedSetting("RawStringFormats");
    TRY_FORMAT(Style, CodeFiles, ReflowComments, { true });
    TRY_FORMAT(Style, CodeFiles, SortIncludes, { true });
    writeNotApplicableSetting("SortJavaStaticImport");
    TRY_FORMAT(Style, CodeFiles, SortUsingDeclarations);
    TRY_FORMAT(Style, CodeFiles, SpaceAfterCStyleCast);
    TRY_FORMAT(Style, CodeFiles, SpaceAfterLogicalNot);
    TRY_FORMAT(Style, CodeFiles, SpaceAfterTemplateKeyword);
    TRY_FORMAT(Style, CodeFiles, SpaceAroundPointerQualifiers);
    TRY_FORMAT(Style, CodeFiles, SpaceBeforeAssignmentOperators);
    TRY_FORMAT(Style, CodeFiles, SpaceBeforeCaseColon);
    TRY_FORMAT(Style, CodeFiles, SpaceBeforeCpp11BracedList);
    TRY_FORMAT(Style, CodeFiles, SpaceBeforeCtorInitializerColon);
    TRY_FORMAT(Style, CodeFiles, SpaceBeforeInheritanceColon);
    TRY_FORMAT(Style, CodeFiles, SpaceBeforeParens);
    TRY_FORMAT(Style, CodeFiles, SpaceBeforeRangeBasedForLoopColon);
    TRY_FORMAT(Style, CodeFiles, SpaceBeforeSquareBrackets);
    TRY_FORMAT(Style, CodeFiles, SpaceInEmptyBlock);
    TRY_FORMAT(Style, CodeFiles, SpaceInEmptyParentheses);

    tryFormat<int>(Style, CodeFiles, "SpacesBeforeTrailingComments",
                   { 0, 1, 2, 3, 4, 5, 6, 7, 8 },
                   memberSetter(&FormatStyle::SpacesBeforeTrailingComments),
                   Style.SpacesBeforeTrailingComments);

    TRY_FORMAT(Style, CodeFiles, SpacesInAngles);
    TRY_FORMAT(Style, CodeFiles, SpacesInConditionalStatement);
    TRY_FORMAT(Style, CodeFiles, SpacesInContainerLiterals);
    TRY_FORMAT(Style, CodeFiles, SpacesInCStyleCastParentheses);
    TRY_FORMAT(Style, CodeFiles, SpacesInContainerLiterals);
    TRY_FORMAT(Style, CodeFiles, SpacesInParentheses);
    TRY_FORMAT(Style, CodeFiles, SpacesInSquareBrackets);
    TRY_FORMAT(Style, CodeFiles, Standard);
    writeUnguessableSetting("StatementMacros");
    writeUnguessableSetting("StatementAttributeLikeMacros");
    writeUnguessableSetting("TypenameMacros");
    writeUnguessableSetting("WhitespaceSensitiveMacros");
  } catch (std::exception& e) {
    errs() << e.what() << "\n";
    return 1;
  }

  return 0;
}
