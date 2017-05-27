#include "../stdafx.h"
#include "GenCP1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGenCP1::CGenCP1() {
}

CGenCP1::~CGenCP1() {
}

void CGenCP1::LoadConfigLine(CString * sN, CString * sV, int idata, float fdata) {
	CheckVar(sN, sV, "cantus_high", &cantus_high);

	CGenCA1::LoadConfigLine(sN, sV, idata, fdata);
}

void CGenCP1::InitCP() {
	ac.resize(av_cnt);
	acc.resize(av_cnt);
	apc.resize(av_cnt);
	apcc.resize(av_cnt);
	asmooth.resize(av_cnt);
	aleap.resize(av_cnt);
	ahm.resize(av_cnt);
	ahm2.resize(av_cnt);
	anflags.resize(av_cnt);
	anflagsc.resize(av_cnt);
}

void CGenCP1::MakeNewCP() {
	// Set pitch limits
	for (int i = 0; i < c_len; ++i) {
		min_c[i] = ac[0][i] + min_interval;
		max_c[i] = ac[0][i] + max_interval;
	}
	// Convert limits to chromatic
	for (int i = 0; i < c_len; ++i) {
		min_cc[i] = C_CC(min_c[i], tonic_cur, minor_cur);
		max_cc[i] = C_CC(max_c[i], tonic_cur, minor_cur);
	}
	if (random_seed) {
		RandCantus(ac[1], 0, c_len);
	}
	else {
		FillCantus(acc[1], 0, c_len, min_cc);
	}
}

void CGenCP1::MultiCPInit() {
	MakeNewCP();
	sp1 = 1; // Start of search window
	sp2 = sp1 + s_len; // End of search window
	if (sp2 > c_len - 1) sp2 = c_len - 1;
	// Record window
	wid = 0;
	wpos1[wid] = sp1;
	wpos2[wid] = sp2;
	// Add last note if this is last window
	ep1 = sp1;
	ep2 = sp2; // End of evaluation window
	if (ep2 == c_len - 1) ep2 = c_len;
	p = sp2 - 1; // Minimal position in array to cycle
}

void CGenCP1::ScanCPInit() {
	// Get cantus size
	//if (task != tGen) c_len = scantus->size();
	// Resize global vectors
	for (int i = 0; i < av_cnt; ++i) {
		ac[i].resize(c_len); // cantus (diatonic)
		acc[i].resize(c_len); // cantus (chromatic)
		anflags[i].resize(c_len, vector<int>(MAX_FLAGS)); // Flags for each note
		anflagsc[i].resize(c_len); // number of flags for each note
		acc_old[i].resize(c_len); // Cantus diatonic saved for SWA
		apc[i].resize(c_len);
		apcc[i].resize(c_len);
		aleap[i].resize(c_len);
		asmooth[i].resize(c_len);
		ahm[i].resize(c_len);
		ahm2[i].resize(c_len);
		for (int x = 0; x < c_len; ++x) {
			ahm[i][x].resize(3);
			ahm2[i][x].resize(3);
		}
	}
	fpenalty.resize(MAX_FLAGS);
	wpos1.resize(c_len / s_len + 1);
	wpos2.resize(c_len / s_len + 1);
	min_c.resize(c_len);
	max_c.resize(c_len);
	min_cc.resize(c_len);
	max_cc.resize(c_len);
	accepted4.resize(MAX_WIND); // number of accepted canti per window
	accepted5.resize(MAX_WIND); // number of canti with neede flags per window
	flags.resize(MAX_FLAGS); // Flags for whole cantus
	fstat.resize(MAX_FLAGS); // number of canti with each flag
	fcor.resize(MAX_FLAGS, vector<long long>(MAX_FLAGS)); // Flags correlation matrix
	seed_cycle = 0; // Number of cycles in case of random_seed
	reseed_count = 0;
	nstat.resize(MAX_NOTE);
	nstat2.resize(MAX_NOTE);
	nstat3.resize(MAX_NOTE);
	cycle = 0;
	wscans.resize(MAX_WIND); // number of full scans per window
	wcount = 1; // Number of windows created
	accepted = 0;
	accepted2 = 0;
	accepted3 = 0;
	// Can we skip flags?
	skip_flags = !calculate_blocking && !calculate_correlation && !calculate_stat;
	// Initialize fblock if calculation is needed
	if (calculate_blocking) {
		fblock = vector<vector<vector<long>>>(MAX_WIND, vector<vector<long>>(MAX_FLAGS, vector<long>(MAX_FLAGS)));
	}
	// Init best rejected results
	if (best_rejected) {
		rcycle = 0;
		accept_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		rpenalty_min = MAX_PENALTY;
	}
}

void CGenCP1::ScanCP(int t, int v) {
	CString st, st2;
	int finished = 0;
	// Load master parameters
	task = t;
	svoice = v;

	ScanCPInit();
	if (task == tGen) MultiCantusInit();
	//else SingleCantusInit();
	if (FailWindowsLimit()) return;
	// Analyze combination
check:
	while (true) {
		//LogCantus(cc);
		if (FailNoteRepeat(acc[1], ep1 - 1, ep2 - 1)) goto skip;
		++accepted3;
		if (need_exit && task != tEval) break;
		// Show status
		if (accepted3 % 100000 == 0) ShowScanStatus();
		if (FailDiatonic(ac[1], acc[1], 0, ep2, minor_cur)) goto skip;
		GetPitchClass(ac[1], pc, 0, ep2);
		if (minor_cur && FailMinor()) goto skip;
		//if (MatchVectors(cc, test_cc, 0, 2)) 
		//WriteLog(1, "Found");
		//if (FailLastNotes(pc, ep2)) goto skip;
		if (FailNoteSeq(apc[1], 0, ep2)) goto skip;
		if (FailIntervals(ep2, apc[1])) goto skip;
		if (FailLeapSmooth(ac[1], ep2, leap, smooth)) goto skip;
		if (FailOutstandingRepeat(acc[1], leap, ep2, repeat_steps2, 2, 76)) goto skip;
		if (FailOutstandingRepeat(acc[1], leap, ep2, repeat_steps3, 3, 36)) goto skip;
		if (FailLongRepeat(acc[1], leap, ep2, repeat_steps5, 5, 72)) goto skip;
		if (FailLongRepeat(acc[1], leap, ep2, repeat_steps7, 7, 73)) goto skip;
		GetMelodyInterval(acc[1], 0, ep2);
		// Calculate diatonic limits
		nmind = CC_C(nmin, tonic_cur, minor_cur);
		nmaxd = CC_C(nmax, tonic_cur, minor_cur);
		GlobalFill(ac[1], ep2, nstat2);
		if (FailStagnation(acc[1], nstat, ep2)) goto skip;
		if (FailMultiCulm(acc[1], ep2)) goto skip;
		if (FailFirstNotes(apc[1], ep2)) goto skip;
		if (FailLeap(ac[1], ep2, leap, smooth, nstat2, nstat3)) goto skip;
		//if (FailMelodyHarm(pc, 0, ep2)) goto skip;

		SaveBestRejected();
		// If we are window-scanning
		if ((task == tGen || task == tCor) && method == mScan) {
			++accepted2;
			CalcFlagStat();
			if (FailFlagBlock()) goto skip;
			if (FailAccept()) goto skip;
			++accepted4[wid];
			// If this is not last window, go to next window
			if (ep2 < c_len) {
				NextWindow();
				goto check;
			}
			// Check random_choose
			if (random_choose < 100) if (rand2() >= (float)RAND_MAX*random_choose / 100.0) goto skip;
		}
		// Calculate rules penalty if we evaluate or correct cantus without full scan
		else {
			CalcRpenalty();
		}
		// Accept cantus
		++accepted;
		TimeBestRejected();
		if (method == mScan && task == tCor) {
			SaveCantus();
		}
		else if (method == mSWA && task == tCor) {
			SaveCantusIfRp();
		}
		else {
			if (task == tGen && accept_reseed) {
				if (clib_vs.Insert(cc)) {
					if (SendCantus()) break;
					ReseedCantus();
					// Start evaluating without scan
					goto check;
				}
				else {
					++cantus_ignored;
				}
			}
			else {
				if (SendCantus()) break;
			}
			// Exit if this is evaluation
			if (task == tEval) return;
		}
	skip:
		ScanLeft(finished);
		if (finished) {
			// Sliding Windows Approximation
			if (method == mSWA) {
				if (NextSWA()) break;
			}
			// Finish if this is last variant in first window and not SWA
			else if ((p == 1) || (wid == 0)) {
				// If we started from random seed, allow one more full cycle
				if (random_seed) {
					if (seed_cycle) {
						// Infinitely cycle through ranges
						if (random_range && accept_reseed) {
							ReseedCantus();
							// Start evaluating without scan
							goto check;
						}
						break;
					}
					// Dont show log if we are reseeding after each accept
					if (!accept_reseed) WriteLog(3, "Random seed allows one more full cycle: restarting");
					++seed_cycle;
				}
				else break;
			}
			BackWindow();
			// Clear flag to prevent coming here again
			finished = 0;
			// Goto next variant calculation
			goto skip;
		} // if (finished)
		ScanRight();
	}
	if (accepted3 > 100000) ShowScanStatus();
	WriteFlagCor();
	ShowFlagStat();
	ShowFlagBlock();
}

void CGenCP1::Generate() {
	CString st;
	InitCantus();
	LoadCantus(midi_file);
	if (cantus.size() < 1) return;
	// Choose cantus to use
	cantus_id = randbw(0, cantus.size() - 1);
	// Get key
	GetCantusKey(cantus[cantus_id]);
	if (tonic_cur == -1) return;
	CalcCcIncrement();
	// Load first voice
	acc[0] = cantus[cantus_id];
	// Set uniform length of each cantus note
	cc_len.resize(c_len);
	cc_tempo.resize(c_len);
	real_len = c_len;
	for (int i = 0; i < c_len; ++i) cc_len[i] = 1;
	// Generate second voice
	ScanCP(tGen, 0);
}
