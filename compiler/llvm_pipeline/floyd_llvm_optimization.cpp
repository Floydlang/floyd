//
//  floyd_llvm_optimization.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-09-08.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

static const bool k_trace_before = false;
static const bool k_trace_after = false;

#include "floyd_llvm_optimization.h"


#include "floyd_llvm_helpers.h"


#include "llvm/CodeGen/CommandFlags.h"



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

#include "llvm/Bitstream/BitstreamWriter.h"




#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
//#include "llvm/CodeGen/CommandFlags.inc"
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





// LTO = Link time optimizations
// PGO = profile guided optimization
// SLP Vectorizer (Apr 2013): Stands for superword-level paralellism


const bool DisableInline = false;

const bool DisableOptimizations = false;

const bool StandardLinkOpts = true;	//	("Include the standard link time optimizations"));

/*
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
*/

const bool DisableLoopUnrolling = false;	//	desc("Disable loop unrolling in all relevant passes"),

// cl::desc("Disable the slp vectorization pass"),
const bool DisableSLPVectorization = false;

const bool DisableSimplifyLibCalls = false;


const bool EnableDebugify = false;
const bool DebugifyEach = false;


const bool Coroutines = false;

const bool StripDebug = false;


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
	if (false)
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
	Builder.DisableUnrollLoops = DisableLoopUnrolling ? DisableLoopUnrolling : OptLevel == 0;

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





namespace floyd {


void optimize_module_mutating(llvm_instance_t& instance, std::unique_ptr<llvm::Module>& module, const compiler_settings_t& settings){
	if(k_trace_before){
		QUARK_TRACE(print_module(*module));
	}

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

	// Strip debug info before running the verifier.
	if (StripDebug)
		StripDebugInfo(*M);

	// Immediately run the verifier to catch any problems before starting up the
	// pass pipelines.  Otherwise we can crash on broken code during
	// doInitialization().
	if (verifyModule(*M, &errs())) {
		errs() << "error: input module is broken!\n";
		throw std::exception();
	}

	Triple ModuleTriple(M->getTargetTriple());
	TargetMachine *Machine = instance.target.target_machine;

	std::string CPUStr = Machine->getTargetCPU().str();
	std::string FeaturesStr = Machine->getTargetFeatureString().str();
	const TargetOptions Options = codegen::InitTargetOptionsFromCodeGenFlags();

	TargetMachine* TM = Machine;

	// Override function attributes based on CPUStr, FeaturesStr, and command line
	// flags.
	codegen::setFunctionAttributes(CPUStr, FeaturesStr, *M);


	// Create a PassManager to hold and optimize the collection of passes we are
	// about to build.
	OptCustomPassManager Passes;

	// Add an appropriate TargetLibraryInfo pass for the module's triple.
	TargetLibraryInfoImpl TLII(ModuleTriple);

	// The -disable-simplify-libcalls flag actually disables all builtin optzns.
	if (DisableSimplifyLibCalls)
		TLII.disableAllFunctions();
	Passes.add(new TargetLibraryInfoWrapperPass(TLII));

	// Add internal analysis passes from the target machine.
	Passes.add(createTargetTransformInfoWrapperPass(TM ? TM->getTargetIRAnalysis() : TargetIRAnalysis()));

	std::unique_ptr<legacy::FunctionPassManager> FPasses;
	if (true /*|| OptLevelO0 || OptLevelO1 || OptLevelO2 || OptLevelOs || OptLevelOz || OptLevelO3*/) {
		FPasses.reset(new legacy::FunctionPassManager(M.get()));
		FPasses->add(createTargetTransformInfoWrapperPass(
				TM ? TM->getTargetIRAnalysis() : TargetIRAnalysis()));
	}

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
	}

	if (settings.optimization_level == eoptimization_level::g_no_optimizations_enable_debugging)
		AddOptimizationPasses(Passes, *FPasses, TM, 0, 0);

	if (settings.optimization_level == eoptimization_level::O1_enable_trivial_optimizations)
		AddOptimizationPasses(Passes, *FPasses, TM, 1, 0);

	if (settings.optimization_level == eoptimization_level::O2_enable_default_optimizations)
		AddOptimizationPasses(Passes, *FPasses, TM, 2, 0);
/*
	if (OptLevelOs)
		AddOptimizationPasses(Passes, *FPasses, TM, 2, 1);

	if (OptLevelOz)
		AddOptimizationPasses(Passes, *FPasses, TM, 2, 2);
*/
	if (settings.optimization_level == eoptimization_level::O3_enable_expensive_optimizations)
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

	if(k_trace_after){
		QUARK_TRACE(print_module(*module));
	}
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



