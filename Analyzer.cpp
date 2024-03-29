#include <exception>
#include <iostream>

#include "StyleSetup.hh"
#include "ControlMenu.hh"
#include "PathUtils.hh"
#include "GraphicsUtils.hh"
#include "PositionResponse.hh"
#include "BetaDecayAnalyzer.hh"
#include "XenonAnalyzer.hh"
#include "PostOfficialAnalyzer.hh"
#include "PlotMakers.hh"
#include "PMTGenerator.hh"
#include "ReSource.hh"
#include "G4toPMT.hh"
#include "PenelopeToPMT.hh"
#include "NuclEvtGen.hh"
#include "AsymmetryCorrections.hh"
#include "OctetSimuCloneManager.hh"

std::vector<RunNum> selectRuns(RunNum r0, RunNum r1, std::string typeSelect) {
	if(typeSelect=="ref") {
		std::vector<RunNum> sruns = CalDBSQL::getCDB()->findRuns("run_type = 'SourceCalib'",r0,r1);
		std::vector<RunNum> rruns;
		for(std::vector<RunNum>::iterator it = sruns.begin(); it != sruns.end(); it++)
			if(CalDBSQL::getCDB()->getGMSRun(*it) == *it)
				rruns.push_back(*it);
		return rruns;
	} else if(typeSelect=="all")
		return CalDBSQL::getCDB()->findRuns("",r0,r1);
	else if(typeSelect=="asym")
		return CalDBSQL::getCDB()->findRuns("run_type = 'Asymmetry'",r0,r1);
	else if(typeSelect=="LED")
		return CalDBSQL::getCDB()->findRuns("run_type = 'LEDCalib'",r0,r1);
	else if(typeSelect=="source")
		return CalDBSQL::getCDB()->findRuns("run_type = 'SourceCalib'",r0,r1);
	else if(typeSelect=="beta")
		return CalDBSQL::getCDB()->findRuns("run_type = 'Asymmetry' AND gate_valve = 'Open'",r0,r1);
	else if(typeSelect=="bg")
		return CalDBSQL::getCDB()->findRuns("run_type = 'Asymmetry' AND gate_valve = 'Closed'",r0,r1);
	return CalDBSQL::getCDB()->findRuns("0 = 1",r0,r1);
}

void mi_EndpointStudy(std::deque<std::string>&, std::stack<std::string>& stack) {
	unsigned int nr = streamInteractor::popInt(stack);
	RunNum r1 = streamInteractor::popInt(stack);
	RunNum r0 = streamInteractor::popInt(stack);
	process_xenon(r0,r1,nr);
}

void mi_EndpointStudySim(std::deque<std::string>&, std::stack<std::string>& stack) {
	unsigned int nRings = streamInteractor::popInt(stack);
	RunNum rsingle = streamInteractor::popInt(stack);
	RunNum r1 = streamInteractor::popInt(stack);
	RunNum r0 = streamInteractor::popInt(stack);
	simulate_xenon(r0,r1,rsingle,nRings);
}

void mi_EndpointStudyReSim(std::deque<std::string>&, std::stack<std::string>& stack) {
	unsigned int nRings = streamInteractor::popInt(stack);
	RunNum r1 = streamInteractor::popInt(stack);
	RunNum r0 = streamInteractor::popInt(stack);
	xenon_posmap(r0,r1,nRings);
}

void mi_PosmapPlot(std::deque<std::string>&, std::stack<std::string>& stack) {
	unsigned int pmid = streamInteractor::popInt(stack);
	if(CalDBSQL::getCDB()->isValid(13883)) {
		OutputManager OM("Foo",getEnvSafe("UCNA_ANA_PLOTS")+"/PositionMaps/Posmap_"+itos(pmid));
		PosPlotter PP(&OM);
		PP.etaPlot(CalDBSQL::getCDB()->getPositioningCorrectorByID(pmid),0.6,1.6);
	} else {
		printf("Invalid CalDB!\n");
	}
}

void mi_nPEPlot(std::deque<std::string>&, std::stack<std::string>& stack) {
	RunNum rn = streamInteractor::popInt(stack);
	PMTCalibrator PCal(rn);
	OutputManager OM("NPE",getEnvSafe("UCNA_ANA_PLOTS")+"/nPE/Run_"+itos(rn));
	PosPlotter PP(&OM);
	PP.npePlot(&PCal);
	OM.write();
}

void mi_PostprocessSources(std::deque<std::string>&, std::stack<std::string>& stack) {
	RunNum r1 = streamInteractor::popInt(stack);
	RunNum r0 = streamInteractor::popInt(stack);
	std::vector<RunNum> C = selectRuns(r0,r1,"source");
	if(!C.size()) {
		printf("No source runs found in Analysis DB; attempting manual scan...\n");
		for(RunNum r = r0; r<= r1; r++)
			reSource(r);
		return;
	}
	printf("Found %i source runs...\n",(int)C.size());
	for(std::vector<RunNum>::iterator it=C.begin(); it!=C.end(); it++)
		reSource(*it);
}

void mi_DumpCalInfo(std::deque<std::string>&, std::stack<std::string>& stack) {
	std::string typeSelect = streamInteractor::popString(stack);
	RunNum r1 = streamInteractor::popInt(stack);
	RunNum r0 = streamInteractor::popInt(stack);
	QFile QOut(getEnvSafe("UCNA_ANA_PLOTS")+"/test/CalDump.txt",false);
	dumpCalInfo(selectRuns(r0,r1,typeSelect),QOut);
}

void mi_dumpPosmap(std::deque<std::string>&, std::stack<std::string>& stack) {
	int pnum = streamInteractor::popInt(stack);
	dumpPosmap(getEnvSafe("UCNA_ANA_PLOTS")+"/PosmapDump/",pnum);
}

void mi_delPosmap(std::deque<std::string>&, std::stack<std::string>& stack) {
	int pnum = streamInteractor::popInt(stack);
	CalDBSQL::getCDB(false)->deletePosmap(pnum);
}

void mi_listPosmaps(std::deque<std::string>&, std::stack<std::string>&) { CalDBSQL::getCDB()->listPosmaps(); }

void mi_processOctet(std::deque<std::string>&, std::stack<std::string>& stack) {
	int octn = streamInteractor::popInt(stack);
	
	OctetSimuCloneManager OSCM("OctetAsym_Offic");
	OutputManager OM("ThisNameIsNotUsedAnywhere",getEnvSafe("UCNA_ANA_PLOTS"));
	
	// simulations input setup
	OSCM.simFile = getEnvSafe("G4OUTDIR")+"/20120823_neutronBetaUnpol/analyzed_";
	//OSCM.simFile = getEnvSafe("G4OUTDIR")+"/20120824_MagF_neutronBetaUnpol/analyzed_";
	//std::string simFile="/home/mmendenhall/geant4/output/20120824_MagF_neutronBetaUnpol/analyzed_";
	//std::string simFile="/home/mmendenhall/geant4/output/thinFoil_neutronBetaUnpol/analyzed_";
	//OSCM.simFile= getEnvSafe("G4OUTDIR")+"/endcap_180_150_neutronBetaUnpol/analyzed_";
	OSCM.simFactor = 1.0;
	OSCM.doPlots = true;
	
	/////////// Geant4 MagF
	OSCM.nTot = 104;
	OSCM.stride = 14;
	
	/////////// Geant4 0823, thinfoil
	//OSCM.nTot = 312;
	//OSCM.stride = 73;
	
	/////////// endcap_180_150
	//OSCM.nTot = 492;
	//OSCM.stride = 73;

	const std::string simOutName = "_Sim0823";
	const std::string simOutputDir=OSCM.outputDir+simOutName;
	
	if(octn < 0) {
		SimBetaDecayAnalyzer BDA_Sim(&OM,simOutputDir);
		BDA_Sim.simPerfectAsym = true;
		if(octn==-1000) {
			BetaDecayAnalyzer BDA(&OM,OSCM.outputDir,RunAccumulator::processedLocation);
			OSCM.combineSims(BDA_Sim,&BDA);
		} else if(octn==-1001) {
			BetaDecayAnalyzer BDA(&OM,OSCM.outputDir,RunAccumulator::processedLocation);
			SimBetaDecayAnalyzer BDA_MC(&OM,simOutputDir,OSCM.baseDir+"/"+simOutputDir+"/"+simOutputDir);
			BDA_MC.compareMCtoData(BDA);
		} else { OSCM.simOct(BDA_Sim,-octn-1); }
	} else {
		BetaDecayAnalyzer BDA(&OM,OSCM.outputDir);
		if(octn==1000) OSCM.combineOcts(BDA);
		else if(octn==1001) OSCM.recalcAllOctets(BDA,false);
		else { OSCM.scanOct(BDA, octn); }
	}
}

/*
void mi_anaOctRange(std::deque<std::string>&, std::stack<std::string>& stack) {
	int octMax = streamInteractor::popInt(stack);
	int octMin = streamInteractor::popInt(stack);
	std::string inPath = getEnvSafe("UCNA_ANA_PLOTS");
	std::string datset = "OctetAsym_Offic_datFid45";
	std::string outPath = inPath+"/"+datset+"/Range_"+itos(octMin)+"-"+itos(octMax);
	
	if(true) {
		OutputManager OM("CorrectedAsym",outPath+"/CorrectAsym/");
		for(AnalysisChoice a = ANCHOICE_A; a <= ANCHOICE_D; ++a) {
			OctetAnalyzer OAdat(&OM, "DataCorrector_"+ctos(choiceLetter(a)), outPath+"/"+datset);
			AsymmetryPlugin* AAdat = new AsymmetryPlugin(&OAdat);
			OAdat.addPlugin(AAdat);
			AAdat->anChoice = a;
			doFullCorrections(*AAdat,OM);
		}
		OM.write();
		OM.setWriteRoot(true);
	}
}
*/

void mi_evis2etrue(std::deque<std::string>&, std::stack<std::string>&) {
	OutputManager OM("Evis2ETrue",getEnvSafe("UCNA_ANA_PLOTS")+"/Evis2ETrue/20120810/");
	G4toPMT g2p;
	g2p.addFile("/home/mmendenhall/geant4/output/20120810_neutronBetaUnpol/analyzed_*.root");
	PMTCalibrator PCal(16000);
	g2p.setCalibrator(PCal);
	SimSpectrumInfo(g2p,OM);
	OM.setWriteRoot(true);
	OM.write();
}

void mi_sourcelog(std::deque<std::string>&, std::stack<std::string>&) { uploadRunSources("UCNA Run Log 2012.txt"); }

void mi_radcor(std::deque<std::string>&, std::stack<std::string>& stack) {
	float Ep = streamInteractor::popFloat(stack);
	int Z = streamInteractor::popInt(stack);
	int A = streamInteractor::popInt(stack);
	makeCorrectionsFile(A,Z,Ep);
}

void mi_showGenerator(std::deque<std::string>&, std::stack<std::string>& stack) {
	std::string sName = streamInteractor::popString(stack);
	OutputManager OMTest("test",getEnvSafe("UCNA_ANA_PLOTS")+"/test/EventGenerators/"+sName+"/");
	NucDecayLibrary NDL(getEnvSafe("UCNA_AUX")+"/NuclearDecays",1e-6);
	PMTCalibrator PCal(21300);
	showSimSpectrum(sName,OMTest,NDL,PCal);
	return;
}

void mi_showCal(std::deque<std::string>&, std::stack<std::string>& stack) {
	RunNum rn = streamInteractor::popInt(stack);
	PMTCalibrator PCal(rn);
}

void mi_makeSimSpectrum(std::deque<std::string>&, std::stack<std::string>& stack) {
	float eMax = streamInteractor::popFloat(stack);
	std::string simName = streamInteractor::popString(stack);
	
	RunNum rn = 16194;
	
	G4toPMT G2P;
	std::string fPath = getEnvSafe("G4OUTDIR")+"/"+simName;
	G2P.addFile(fPath+"/analyzed_*.root");

	PMTCalibrator PCal(rn);
	G2P.setCalibrator(PCal);
	
	OutputManager OM("SimSpectrum",getEnvSafe("UCNA_ANA_PLOTS")+"/SimSpectrum/");
	TH1F* hSpec = OM.registeredTH1F("hSpec","EventSpectrum",200,0,eMax);
	
	G2P.startScan(false);
	while(G2P.nextPoint()) {
		if(G2P.fType >= TYPE_IV_EVENT) continue;
		hSpec->Fill(G2P.getErecon());
	}
	
	double nOrigEvts = 3e6;
	
	hSpec->SetTitle(NULL);
	hSpec->GetXaxis()->SetTitle("Energy [keV]");
	hSpec->GetYaxis()->SetTitle("Events / keV / 1000 decays");
	hSpec->GetYaxis()->SetTitleOffset(1.2);
	hSpec->Scale(1.0e3/nOrigEvts/hSpec->GetBinWidth(1));
	hSpec->Draw();
	OM.printCanvas(simName);
}

void Analyzer(std::deque<std::string> args=std::deque<std::string>()) {
	
	ROOTStyleSetup();
	
	inputRequester exitMenu("Exit Menu",&menutils_Exit);
	inputRequester peek("Show stack",&menutils_PrintStack);
	
	// selection utilities
	NameSelector selectRuntype("Run Type");
	selectRuntype.addChoice("All Runs","all");
	selectRuntype.addChoice("LED Runs","LED");
	selectRuntype.addChoice("Source Runs","source");
	selectRuntype.addChoice("Beta & BG asymmetry runs","asym");
	selectRuntype.addChoice("Beta Runs","beta");
	selectRuntype.addChoice("Background Runs","bg");
	selectRuntype.addChoice("GMS Reference Runs","ref");
	selectRuntype.setDefault("all");
	
	// position map routines and menu
	inputRequester pm_posmap("Generate Position Map",&mi_EndpointStudy);
	pm_posmap.addArg("Start Run");
	pm_posmap.addArg("End Run");
	pm_posmap.addArg("n Rings","12");
	inputRequester pm_posmap_sim("Simulate Position Map",&mi_EndpointStudySim);
	pm_posmap_sim.addArg("Start Run");
	pm_posmap_sim.addArg("End Run");
	pm_posmap_sim.addArg("Single Run","0");
	pm_posmap_sim.addArg("n Rings","12");
	inputRequester pm_posmap_resim("Reupload Position Map",&mi_EndpointStudyReSim);
	pm_posmap_resim.addArg("Start Run");
	pm_posmap_resim.addArg("End Run");
	pm_posmap_resim.addArg("n Rings","12");
	inputRequester posmapLister("List Posmaps",&mi_listPosmaps);
	inputRequester posmapPlot("Plot Position Map",&mi_PosmapPlot);
	posmapPlot.addArg("Posmap ID");
	inputRequester posmapDumper("Dump Posmap",&mi_dumpPosmap);
	posmapDumper.addArg("Posmap ID");
	inputRequester posmapDel("Delete Posmap",&mi_delPosmap);
	posmapDel.addArg("Posmap ID");
	inputRequester nPEPlot("Plot nPE/MeV",&mi_nPEPlot);
	nPEPlot.addArg("Run Number");
	OptionsMenu PMapR("Position Map Routines");
	PMapR.addChoice(&pm_posmap,"gen");
	PMapR.addChoice(&pm_posmap_sim,"sim");
	PMapR.addChoice(&pm_posmap_resim,"rup");
	PMapR.addChoice(&posmapLister,"ls");
	PMapR.addChoice(&posmapPlot,"plot");
	PMapR.addChoice(&posmapDumper,"dump");
	PMapR.addChoice(&posmapDel,"del");
	PMapR.addChoice(&nPEPlot,"npe");
	PMapR.addChoice(&exitMenu,"x");
	
	// postprocessing/plots routines
	inputRequester dumpCalInfo("Dump calibration info to file",&mi_DumpCalInfo);
	dumpCalInfo.addArg("Start Run");
	dumpCalInfo.addArg("End Run");
	dumpCalInfo.addArg(&selectRuntype);
	
	inputRequester showCal("Show run calibration",&mi_showCal);
	showCal.addArg("Run");
	
	inputRequester octetProcessor("Process Octet",&mi_processOctet);
	octetProcessor.addArg("Octet number");

	inputRequester showGenerator("Event generator test",&mi_showGenerator);
	showGenerator.addArg("Generator name");
	
	inputRequester makeSimSpectrum("Sim Spectrum",&mi_makeSimSpectrum);
	makeSimSpectrum.addArg("Sim name");
	makeSimSpectrum.addArg("energy range");
	
	// Posprocessing menu
	OptionsMenu PostRoutines("Postprocessing Routines");
	PostRoutines.addChoice(&showCal,"cal");
	PostRoutines.addChoice(&dumpCalInfo,"dcl");
	PostRoutines.addChoice(&octetProcessor,"oct");
	PostRoutines.addChoice(&showGenerator,"evg");
	PostRoutines.addChoice(&makeSimSpectrum,"mks");
	PostRoutines.addChoice(&exitMenu,"x");
	
	// sources
	inputRequester postSources("Fit source data",&mi_PostprocessSources);
	postSources.addArg("Start Run");
	postSources.addArg("End Run");
	inputRequester uploadSources("Upload runlog sources",&mi_sourcelog);
	// evis2etrue
	inputRequester evis2etrue("Caluculate eVis->eTrue curves",&mi_evis2etrue);
	// radiative corrections
	inputRequester radcor("Make radiative corrections table",&mi_radcor);
	radcor.addArg("A","1");
	radcor.addArg("Z","1");
	radcor.addArg("Endpoint",dtos(neutronBetaEp));
	
	// main menu
	OptionsMenu OM("Analyzer Main Menu");
	OM.addChoice(&PMapR,"pmap");
	OM.addChoice(&PostRoutines,"pr");
	OM.addChoice(&postSources,"sr");
	OM.addChoice(&uploadSources,"us");
	OM.addChoice(&evis2etrue,"ev");
	OM.addChoice(&radcor,"rc");
	OM.addChoice(&exitMenu,"x");
	OM.addSynonym("x","exit");
	OM.addSynonym("x","quit");
	OM.addSynonym("x","bye");
	OM.addChoice(&peek,"peek",SELECTOR_HIDDEN);
	
	std::stack<std::string> stack;
	OM.doIt(args,stack);
	
	printf("\n\n\n>>>>> Goodbye. <<<<<\n\n\n");
}

int main(int argc, char *argv[]) {
	std::deque<std::string> args;
	for(int i=1; i<argc; i++)
		args.push_back(argv[i]);
	Analyzer(args);
	return 0;
}
