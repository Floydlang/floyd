//
//  floyd_llvm_optimization.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-09-08.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_optimization.h"


#include "floyd_llvm_helpers.h"

#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/PassManager.h>

#include "llvm/Support/TargetRegistry.h"


#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include "llvm/Bitcode/BitstreamWriter.h"




#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/CodeGen/CommandFlags.inc"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LegacyPassNameParser.h"
#include "llvm/IR/Module.h"
//#include "llvm/IR/RemarkStreamer.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/LinkAllIR.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Coroutines.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <algorithm>
#include <memory>
using namespace llvm;
//using namespace opt_tool;




// The OptimizationList is automatically populated with registered Passes by the
// PassNameParser.
//
static cl::list<const PassInfo*, bool, PassNameParser>
PassList(cl::desc("Optimizations available:"));

// This flag specifies a textual description of the optimization pass pipeline
// to run over the module. This flag switches opt to use the new pass manager
// infrastructure, completely disabling all of the flags specific to the old
// pass management.
static cl::opt<std::string> PassPipeline(
		"passes",
		cl::desc("A textual description of the pass pipeline for optimizing"),
		cl::Hidden);

// Other command line options...
//
static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input bitcode file>"),
		cl::init("-"), cl::value_desc("filename"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Override output filename"),
							 cl::value_desc("filename"));

static cl::opt<bool>
Force("f", cl::desc("Enable binary output on terminals"));

static cl::opt<bool>
PrintEachXForm("p", cl::desc("Print module after each transformation"));

static cl::opt<bool>
NoOutput("disable-output",
				 cl::desc("Do not write result bitcode file"), cl::Hidden);

static cl::opt<bool>
OutputAssembly("S", cl::desc("Write output as LLVM assembly"));

static cl::opt<bool>
		OutputThinLTOBC("thinlto-bc",
										cl::desc("Write output as ThinLTO-ready bitcode"));

static cl::opt<bool>
		SplitLTOUnit("thinlto-split-lto-unit",
								 cl::desc("Enable splitting of a ThinLTO LTOUnit"));

static cl::opt<std::string> ThinLinkBitcodeFile(
		"thin-link-bitcode-file", cl::value_desc("filename"),
		cl::desc(
				"A file in which to write minimized bitcode for the thin link only"));

static cl::opt<bool>
NoVerify("disable-verify", cl::desc("Do not run the verifier"), cl::Hidden);

static cl::opt<bool>
VerifyEach("verify-each", cl::desc("Verify after each transform"));

static cl::opt<bool>
		DisableDITypeMap("disable-debug-info-type-map",
										 cl::desc("Don't use a uniquing type map for debug info"));

static cl::opt<bool>
StripDebug("strip-debug",
					 cl::desc("Strip debugger symbol info from translation unit"));

static cl::opt<bool>
		StripNamedMetadata("strip-named-metadata",
											 cl::desc("Strip module-level named metadata"));

static cl::opt<bool> DisableInline("disable-inlining",
																	 cl::desc("Do not run the inliner pass"));

static cl::opt<bool>
DisableOptimizations("disable-opt",
										 cl::desc("Do not run any optimization passes"));

static cl::opt<bool>
StandardLinkOpts("std-link-opts",
								 cl::desc("Include the standard link time optimizations"));

static cl::opt<bool>
OptLevelO0("O0",
	cl::desc("Optimization level 0. Similar to clang -O0"));

static cl::opt<bool>
OptLevelO1("O1",
					 cl::desc("Optimization level 1. Similar to clang -O1"));

static cl::opt<bool>
OptLevelO2("O2",
					 cl::desc("Optimization level 2. Similar to clang -O2"));

static cl::opt<bool>
OptLevelOs("Os",
					 cl::desc("Like -O2 with extra optimizations for size. Similar to clang -Os"));

static cl::opt<bool>
OptLevelOz("Oz",
					 cl::desc("Like -Os but reduces code size further. Similar to clang -Oz"));

static cl::opt<bool>
OptLevelO3("O3",
					 cl::desc("Optimization level 3. Similar to clang -O3"));

static cl::opt<unsigned>
CodeGenOptLevel("codegen-opt-level",
								cl::desc("Override optimization level for codegen hooks"));

static cl::opt<std::string>
TargetTriple("mtriple", cl::desc("Override target triple for module"));

static cl::opt<bool>
DisableLoopUnrolling("disable-loop-unrolling",
										 cl::desc("Disable loop unrolling in all relevant passes"),
										 cl::init(false));

static cl::opt<bool>
DisableSLPVectorization("disable-slp-vectorization",
												cl::desc("Disable the slp vectorization pass"),
												cl::init(false));

static cl::opt<bool> EmitSummaryIndex("module-summary",
																			cl::desc("Emit module summary index"),
																			cl::init(false));

static cl::opt<bool> EmitModuleHash("module-hash", cl::desc("Emit module hash"),
																		cl::init(false));

static cl::opt<bool>
DisableSimplifyLibCalls("disable-simplify-libcalls",
												cl::desc("Disable simplify-libcalls"));

static cl::opt<bool>
Quiet("q", cl::desc("Obsolete option"), cl::Hidden);

static cl::alias
QuietA("quiet", cl::desc("Alias for -q"), cl::aliasopt(Quiet));

static cl::opt<bool>
AnalyzeOnly("analyze", cl::desc("Only perform analysis, no optimization"));

static cl::opt<bool> EnableDebugify(
		"enable-debugify",
		cl::desc(
				"Start the pipeline with debugify and end it with check-debugify"));

static cl::opt<bool> DebugifyEach(
		"debugify-each",
		cl::desc(
				"Start each pass with debugify and end it with check-debugify"));

static cl::opt<std::string>
		DebugifyExport("debugify-export",
									 cl::desc("Export per-pass debugify statistics to this file"),
									 cl::value_desc("filename"), cl::init(""));

static cl::opt<bool>
PrintBreakpoints("print-breakpoints-for-testing",
								 cl::desc("Print select breakpoints location for testing"));

static cl::opt<std::string> ClDataLayout("data-layout",
																				 cl::desc("data layout string to use"),
																				 cl::value_desc("layout-string"),
																				 cl::init(""));

static cl::opt<bool> PreserveBitcodeUseListOrder(
		"preserve-bc-uselistorder",
		cl::desc("Preserve use-list order when writing LLVM bitcode."),
		cl::init(true), cl::Hidden);

static cl::opt<bool> PreserveAssemblyUseListOrder(
		"preserve-ll-uselistorder",
		cl::desc("Preserve use-list order when writing LLVM assembly."),
		cl::init(false), cl::Hidden);

static cl::opt<bool>
		RunTwice("run-twice",
						 cl::desc("Run all passes twice, re-using the same pass manager."),
						 cl::init(false), cl::Hidden);

static cl::opt<bool> DiscardValueNames(
		"discard-value-names",
		cl::desc("Discard names from Value (other than GlobalValue)."),
		cl::init(false), cl::Hidden);

static cl::opt<bool> Coroutines(
	"enable-coroutines",
	cl::desc("Enable coroutine passes."),
	cl::init(false), cl::Hidden);

static cl::opt<bool> RemarksWithHotness(
		"pass-remarks-with-hotness",
		cl::desc("With PGO, include profile count in optimization remarks"),
		cl::Hidden);

static cl::opt<unsigned>
		RemarksHotnessThreshold("pass-remarks-hotness-threshold",
														cl::desc("Minimum profile count required for "
																		 "an optimization remark to be output"),
														cl::Hidden);

static cl::opt<std::string>
		RemarksFilename("pass-remarks-output",
										cl::desc("Output filename for pass remarks"),
										cl::value_desc("filename"));

static cl::opt<std::string>
		RemarksPasses("pass-remarks-filter",
									cl::desc("Only record optimization remarks from passes whose "
													 "names match the given regular expression"),
									cl::value_desc("regex"));

static cl::opt<std::string> RemarksFormat(
		"pass-remarks-format",
		cl::desc("The format used for serializing remarks (default: YAML)"),
		cl::value_desc("format"), cl::init("yaml"));

#if 0
cl::opt<PGOKind>
		PGOKindFlag("pgo-kind", cl::init(NoPGO), cl::Hidden,
								cl::desc("The kind of profile guided optimization"),
								cl::values(clEnumValN(NoPGO, "nopgo", "Do not use PGO."),
													 clEnumValN(InstrGen, "pgo-instr-gen-pipeline",
																			"Instrument the IR to generate profile."),
													 clEnumValN(InstrUse, "pgo-instr-use-pipeline",
																			"Use instrumented profile to guide PGO."),
													 clEnumValN(SampleUse, "pgo-sample-use-pipeline",
																			"Use sampled profile to guide PGO.")));
cl::opt<std::string> ProfileFile("profile-file",
																 cl::desc("Path to the profile."), cl::Hidden);

cl::opt<CSPGOKind> CSPGOKindFlag(
		"cspgo-kind", cl::init(NoCSPGO), cl::Hidden,
		cl::desc("The kind of context sensitive profile guided optimization"),
		cl::values(
				clEnumValN(NoCSPGO, "nocspgo", "Do not use CSPGO."),
				clEnumValN(
						CSInstrGen, "cspgo-instr-gen-pipeline",
						"Instrument (context sensitive) the IR to generate profile."),
				clEnumValN(
						CSInstrUse, "cspgo-instr-use-pipeline",
						"Use instrumented (context sensitive) profile to guide PGO.")));
cl::opt<std::string> CSProfileGenFile(
		"cs-profilegen-file",
		cl::desc("Path to the instrumented context sensitive profile."),
		cl::Hidden);
#endif

class OptCustomPassManager : public legacy::PassManager {
//  DebugifyStatsMap DIStatsMap;

public:
	using super = legacy::PassManager;

	void add(Pass *P) override {
		// Wrap each pass with (-check)-debugify passes if requested, making
		// exceptions for passes which shouldn't see -debugify instrumentation.
		bool WrapWithDebugify = DebugifyEach && !P->getAsImmutablePass() &&
														!isIRPrintingPass(P) && !isBitcodeWriterPass(P);
		if (!WrapWithDebugify) {
			super::add(P);
			return;
		}

		// Apply -debugify/-check-debugify before/after each pass and collect
		// debug info loss statistics.
		PassKind Kind = P->getPassKind();
		StringRef Name = P->getPassName();

		// TODO: Implement Debugify for BasicBlockPass, LoopPass.
		switch (Kind) {
			case PT_Function:
//        super::add(createDebugifyFunctionPass());
				super::add(P);
//        super::add(createCheckDebugifyFunctionPass(true, Name, &DIStatsMap));
				break;
			case PT_Module:
//        super::add(createDebugifyModulePass());
				super::add(P);
	//      super::add(createCheckDebugifyModulePass(true, Name, &DIStatsMap));
				break;
			default:
				super::add(P);
				break;
		}
	}

//  const DebugifyStatsMap &getDebugifyStatsMap() const { return DIStatsMap; }
};

static inline void addPass(legacy::PassManagerBase &PM, Pass *P) {
	// Add the pass to the pass manager...
	PM.add(P);

	// If we are verifying all of the intermediate steps, add the verifier...
	if (VerifyEach)
		PM.add(createVerifierPass());
}

/// This routine adds optimization passes based on selected optimization level,
/// OptLevel.
///
/// OptLevel - Optimization Level
static void AddOptimizationPasses(legacy::PassManagerBase &MPM,
																	legacy::FunctionPassManager &FPM,
																	TargetMachine *TM, unsigned OptLevel,
																	unsigned SizeLevel) {
	if (!NoVerify || VerifyEach)
		FPM.add(createVerifierPass()); // Verify that input is correct

	PassManagerBuilder Builder;
	Builder.OptLevel = OptLevel;
	Builder.SizeLevel = SizeLevel;

	if (DisableInline) {
		// No inlining pass
	} else if (OptLevel > 1) {
		Builder.Inliner = createFunctionInliningPass(OptLevel, SizeLevel, false);
	} else {
		Builder.Inliner = createAlwaysInlinerLegacyPass();
	}
	Builder.DisableUnrollLoops = (DisableLoopUnrolling.getNumOccurrences() > 0) ?
															 DisableLoopUnrolling : OptLevel == 0;

	// Check if vectorization is explicitly disabled via -vectorize-loops=false.
	// The flag enables vectorization in the LoopVectorize pass, it is on by
	// default, and if it was disabled, leave it disabled here.
	// Another flag that exists: -loop-vectorize, controls adding the pass to the
	// pass manager. If set, the pass is added, and there is no additional check
	// here for it.
	if (Builder.LoopVectorize)
		Builder.LoopVectorize = OptLevel > 1 && SizeLevel < 2;

	// When #pragma vectorize is on for SLP, do the same as above
	Builder.SLPVectorize =
			DisableSLPVectorization ? false : OptLevel > 1 && SizeLevel < 2;

	if (TM)
		TM->adjustPassManager(Builder);

	if (Coroutines)
		addCoroutinePassesToExtensionPoints(Builder);

/*
	switch (PGOKindFlag) {
	case InstrGen:
		Builder.EnablePGOInstrGen = true;
		Builder.PGOInstrGen = ProfileFile;
		break;
	case InstrUse:
		Builder.PGOInstrUse = ProfileFile;
		break;
	case SampleUse:
		Builder.PGOSampleUse = ProfileFile;
		break;
	default:
		break;
	}

	switch (CSPGOKindFlag) {
	case CSInstrGen:
//    Builder.EnablePGOCSInstrGen = true;
		break;
	case CSInstrUse:
//    Builder.EnablePGOCSInstrUse = true;
		break;
	default:
		break;
	}
*/

	Builder.populateFunctionPassManager(FPM);
	Builder.populateModulePassManager(MPM);
}

static void AddStandardLinkPasses(legacy::PassManagerBase &PM) {
	PassManagerBuilder Builder;
	Builder.VerifyInput = true;
	if (DisableOptimizations)
		Builder.OptLevel = 0;

	if (!DisableInline)
		Builder.Inliner = createFunctionInliningPass();
	Builder.populateLTOPassManager(PM);
}

//===----------------------------------------------------------------------===//
// CodeGen-related helper functions.
//

static CodeGenOpt::Level GetCodeGenOptLevel() {
	if (CodeGenOptLevel.getNumOccurrences())
		return static_cast<CodeGenOpt::Level>(unsigned(CodeGenOptLevel));
	if (OptLevelO1)
		return CodeGenOpt::Less;
	if (OptLevelO2)
		return CodeGenOpt::Default;
	if (OptLevelO3)
		return CodeGenOpt::Aggressive;
	return CodeGenOpt::None;
}

// Returns the TargetMachine instance or zero if no triple is provided.
static TargetMachine* GetTargetMachine(Triple TheTriple, StringRef CPUStr,
																			 StringRef FeaturesStr,
																			 const TargetOptions &Options) {
	std::string Error;
	const Target *TheTarget = TargetRegistry::lookupTarget(MArch, TheTriple,
																												 Error);
	// Some modules don't specify a triple, and this is okay.
	if (!TheTarget) {
		return nullptr;
	}

	return TheTarget->createTargetMachine(TheTriple.getTriple(), CPUStr,
																				FeaturesStr, Options, getRelocModel(),
																				getCodeModel(), GetCodeGenOptLevel());
}



namespace floyd {


void optimize_module_mutating(llvm_instance_t& instance, std::unique_ptr<llvm::Module>& module){
	QUARK_TRACE(print_module(*module));

	auto& Context = instance.context;
	std::unique_ptr<Module>& M = module;
/*
	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmPrinters();
	InitializeAllAsmParsers();
*/

	// Initialize passes
	PassRegistry& Registry = *PassRegistry::getPassRegistry();
	initializeCore(Registry);
	initializeCoroutines(Registry);
	initializeScalarOpts(Registry);
	initializeObjCARCOpts(Registry);
	initializeVectorization(Registry);
	initializeIPO(Registry);
	initializeAnalysis(Registry);
	initializeTransformUtils(Registry);
	initializeInstCombine(Registry);
	initializeAggressiveInstCombine(Registry);
	initializeInstrumentation(Registry);
	initializeTarget(Registry);
	// For codegen passes, only passes that do IR to IR transformation are
	// supported.
	initializeExpandMemCmpPassPass(Registry);
	initializeScalarizeMaskedMemIntrinPass(Registry);
	initializeCodeGenPreparePass(Registry);
	initializeAtomicExpandPass(Registry);
	initializeRewriteSymbolsLegacyPassPass(Registry);
	initializeWinEHPreparePass(Registry);
	initializeDwarfEHPreparePass(Registry);
	initializeSafeStackLegacyPassPass(Registry);
	initializeSjLjEHPreparePass(Registry);
	initializeStackProtectorPass(Registry);
	initializePreISelIntrinsicLoweringLegacyPassPass(Registry);
	initializeGlobalMergePass(Registry);
	initializeIndirectBrExpandPassPass(Registry);
	initializeInterleavedLoadCombinePass(Registry);
	initializeInterleavedAccessPass(Registry);
	initializeEntryExitInstrumenterPass(Registry);
	initializePostInlineEntryExitInstrumenterPass(Registry);
	initializeUnreachableBlockElimLegacyPassPass(Registry);
	initializeExpandReductionsPass(Registry);
	initializeWasmEHPreparePass(Registry);
	initializeWriteBitcodePassPass(Registry);
//???  initializeHardwareLoopsPass(Registry);


	SMDiagnostic Err;

/*
	// Load the input module...
	std::unique_ptr<Module> M =
			parseIRFile(InputFilename, Err, Context, !NoVerify, ClDataLayout);
*/

	// Strip debug info before running the verifier.
	if (StripDebug)
		StripDebugInfo(*M);

	// Erase module-level named metadata, if requested.
	if (StripNamedMetadata) {
		while (!M->named_metadata_empty()) {
			NamedMDNode *NMD = &*M->named_metadata_begin();
			M->eraseNamedMetadata(NMD);
		}
	}

	// If we are supposed to override the target triple or data layout, do so now.
//	if (!TargetTriple.empty())
//		M->setTargetTriple(Triple::normalize(TargetTriple));

	// Immediately run the verifier to catch any problems before starting up the
	// pass pipelines.  Otherwise we can crash on broken code during
	// doInitialization().
	if (verifyModule(*M, &errs())) {
		errs() << "error: input module is broken!\n";
		throw std::exception();
	}

	// Figure out what stream we are supposed to write to...
	std::unique_ptr<ToolOutputFile> Out;

	Triple ModuleTriple(M->getTargetTriple());
	TargetMachine *Machine = instance.target.target_machine;

	std::string CPUStr = Machine->getTargetCPU();
	std::string FeaturesStr = Machine->getTargetFeatureString();
	const TargetOptions Options = InitTargetOptionsFromCodeGenFlags();


	TargetMachine* TM = Machine;

	// Override function attributes based on CPUStr, FeaturesStr, and command line
	// flags.
	setFunctionAttributes(CPUStr, FeaturesStr, *M);


	if (OutputThinLTOBC)
		M->addModuleFlag(Module::Error, "EnableSplitLTOUnit", SplitLTOUnit);

/*

	if (PassPipeline.getNumOccurrences() > 0) {
		OutputKind OK = OK_NoOutput;
		if (!NoOutput)
			OK = OutputAssembly
							 ? OK_OutputAssembly
							 : (OutputThinLTOBC ? OK_OutputThinLTOBitcode : OK_OutputBitcode);

		VerifierKind VK = VK_VerifyInAndOut;
		if (NoVerify)
			VK = VK_NoVerifier;
		else if (VerifyEach)
			VK = VK_VerifyEachPass;

		// The user has asked to use the new pass manager and provided a pipeline
		// string. Hand off the rest of the functionality to the new code for that
		// layer.
		return runPassPipeline(argv[0], *M, TM.get(), Out.get(), ThinLinkOut.get(),
													 RemarksFile.get(), PassPipeline, OK, VK,
													 PreserveAssemblyUseListOrder,
													 PreserveBitcodeUseListOrder, EmitSummaryIndex,
													 EmitModuleHash, EnableDebugify)
							 ? 0
							 : 1;
	}
*/

	// Create a PassManager to hold and optimize the collection of passes we are
	// about to build.
	OptCustomPassManager Passes;
	bool AddOneTimeDebugifyPasses = EnableDebugify && !DebugifyEach;

	// Add an appropriate TargetLibraryInfo pass for the module's triple.
	TargetLibraryInfoImpl TLII(ModuleTriple);

	// The -disable-simplify-libcalls flag actually disables all builtin optzns.
	if (DisableSimplifyLibCalls)
		TLII.disableAllFunctions();
	Passes.add(new TargetLibraryInfoWrapperPass(TLII));

	// Add internal analysis passes from the target machine.
	Passes.add(createTargetTransformInfoWrapperPass(TM ? TM->getTargetIRAnalysis()
																										 : TargetIRAnalysis()));

/*
	if (AddOneTimeDebugifyPasses)
		Passes.add(createDebugifyModulePass());
*/

	std::unique_ptr<legacy::FunctionPassManager> FPasses;
	if (true || OptLevelO0 || OptLevelO1 || OptLevelO2 || OptLevelOs || OptLevelOz ||
			OptLevelO3) {
		FPasses.reset(new legacy::FunctionPassManager(M.get()));
		FPasses->add(createTargetTransformInfoWrapperPass(
				TM ? TM->getTargetIRAnalysis() : TargetIRAnalysis()));
	}

/*
	if (PrintBreakpoints) {
		// Default to standard output.
		if (!Out) {
			if (OutputFilename.empty())
				OutputFilename = "-";

			std::error_code EC;
			Out = std::make_unique<ToolOutputFile>(OutputFilename, EC,
																							sys::fs::OF_None);
			if (EC) {
				errs() << EC.message() << '\n';
				return 1;
			}
		}
		Passes.add(createBreakpointPrinter(Out->os()));
		NoOutput = true;
	}
*/
	if (TM) {
		// FIXME: We should dyn_cast this when supported.
		auto &LTM = static_cast<LLVMTargetMachine &>(*TM);
		Pass *TPC = LTM.createPassConfig(Passes);
		Passes.add(TPC);
	}

/*
	// Create a new optimization pass for each one specified on the command line
	for (unsigned i = 0; i < PassList.size(); ++i) {
		if (StandardLinkOpts &&
				StandardLinkOpts.getPosition() < PassList.getPosition(i)) {
			AddStandardLinkPasses(Passes);
			StandardLinkOpts = false;
		}

		if (OptLevelO0 && OptLevelO0.getPosition() < PassList.getPosition(i)) {
			AddOptimizationPasses(Passes, *FPasses, TM.get(), 0, 0);
			OptLevelO0 = false;
		}

		if (OptLevelO1 && OptLevelO1.getPosition() < PassList.getPosition(i)) {
			AddOptimizationPasses(Passes, *FPasses, TM.get(), 1, 0);
			OptLevelO1 = false;
		}

		if (OptLevelO2 && OptLevelO2.getPosition() < PassList.getPosition(i)) {
			AddOptimizationPasses(Passes, *FPasses, TM.get(), 2, 0);
			OptLevelO2 = false;
		}

		if (OptLevelOs && OptLevelOs.getPosition() < PassList.getPosition(i)) {
			AddOptimizationPasses(Passes, *FPasses, TM.get(), 2, 1);
			OptLevelOs = false;
		}

		if (OptLevelOz && OptLevelOz.getPosition() < PassList.getPosition(i)) {
			AddOptimizationPasses(Passes, *FPasses, TM.get(), 2, 2);
			OptLevelOz = false;
		}

		if (OptLevelO3 && OptLevelO3.getPosition() < PassList.getPosition(i)) {
			AddOptimizationPasses(Passes, *FPasses, TM.get(), 3, 0);
			OptLevelO3 = false;
		}

		const PassInfo *PassInf = PassList[i];
		Pass *P = nullptr;
		if (PassInf->getNormalCtor())
			P = PassInf->getNormalCtor()();
		else
			errs() << "" << ": cannot create pass: "
						 << PassInf->getPassName() << "\n";
		if (P) {
			PassKind Kind = P->getPassKind();
			addPass(Passes, P);
		}

		if (PrintEachXForm)
			Passes.add(
					createPrintModulePass(errs(), "", PreserveAssemblyUseListOrder));
	}
*/

	if (StandardLinkOpts) {
		AddStandardLinkPasses(Passes);
		StandardLinkOpts = false;
	}

	if (OptLevelO0)
		AddOptimizationPasses(Passes, *FPasses, TM, 0, 0);

	if (OptLevelO1)
		AddOptimizationPasses(Passes, *FPasses, TM, 1, 0);

	if (OptLevelO2)
		AddOptimizationPasses(Passes, *FPasses, TM, 2, 0);

	if (OptLevelOs)
		AddOptimizationPasses(Passes, *FPasses, TM, 2, 1);

	if (OptLevelOz)
		AddOptimizationPasses(Passes, *FPasses, TM, 2, 2);

	//???
	if (true || OptLevelO3)
		AddOptimizationPasses(Passes, *FPasses, TM, 3, 0);

	if (FPasses) {
		FPasses->doInitialization();
		for (Function &F : *M)
			FPasses->run(F);
		FPasses->doFinalization();
	}

	// Check that the module is well formed on completion of optimization
	Passes.add(createVerifierPass());


	// Now that we have all of the passes ready, run them.
	Passes.run(*M);
}


/*
https://stackoverflow.com/questions/31279623/llvm-optimization-using-c-api

//take string "llvm" (LLVM IR) and return "output_llvm" (optimized LLVM IR)

static string optimize(string llvm) {
    LLVMContext &ctx = getGlobalContext();
    SMDiagnostic err;
    Module *ir = ParseIR(MemoryBuffer::getMemBuffer(llvm), err, ctx);
    PassManager *pm = new PassManager();
    PassManagerBuilder builder;
    builder.OptLevel = 3;
    builder.populateModulePassManager(*pm);
    pm->run(*ir);
    delete pm;
    string output_llvm;
    raw_string_ostream buff(output_llvm);
    ir->print(buff, NULL);
    return output_llvm;
}

PassManager *pm = new PassManager();
int optLevel = 3;
int sizeLevel = 0;
PassManagerBuilder builder;
builder.OptLevel = optLevel;
builder.SizeLevel = sizeLevel;
builder.Inliner = createFunctionInliningPass(optLevel, sizeLevel);
builder.DisableUnitAtATime = false;
builder.DisableUnrollLoops = false;
builder.LoopVectorize = true;
builder.SLPVectorize = true;
builder.populateModulePassManager(*pm);
pm->run(*module);

FunctionPassManager *fpm = new FunctionPassManager(module);
// add several passes
fpm->doInitialization();
for (Function &f : *ir)
    fpm->run(f);
fpm->doFinalization();


*/


} // floyd



