CP2R::CP2R() {
	// Data ready
	data_ready.resize(MAX_DATA_READY);
	data_ready_persist.resize(MAX_DATA_READY_PERSIST);
	warn_data_ready.resize(MAX_DATA_READY);
	warn_data_ready_persist.resize(MAX_DATA_READY_PERSIST);
}

CP2R::~CP2R() {
}

void CP2R::CreateLinks() {
	SET_READY(DR_fli);
	// Set first steps in case there is pause
	for (int v = 0; v < av_cnt; ++v) {
		int prev_note = -1;
		int lpos = 0;
		int l = 0;
		minl[v] = 10000;
		maxl[v] = 0;
		for (int i = 0; i < ep2; ++i) {
			if (prev_note != cc[v][i] || retr[v][i]) {
				// Save linked note length
				if (prev_note != -1) {
					llen[v][lpos - 1] = l;
					rlen[v][lpos - 1] = l;
					if (minl[v] > l) minl[v] = l;
					if (maxl[v] < l) maxl[v] = l;
					l = 0;
				}
				prev_note = cc[v][i];
				fli[v][lpos] = i;
				++lpos;
			}
			fli2[v][lpos - 1] = i;
			bli[v][i] = lpos - 1;
			l++;
		}
		fli_size[v] = lpos;
		llen[v][lpos - 1] = l;
		rlen[v][lpos - 1] = l;
	}
}

void CP2R::GetVca() {
	SET_READY(DR_vca);
	// Set first steps in case there is pause
	for (int s = 0; s < ep2; ++s) {
		vca[s] = 0;
		hva[s] = 0;
		lva[s] = av_cnt - 1;
		for (int v = 0; v < av_cnt; ++v) {
			if (cc[v][s]) {
				++vca[s];
				if (v > hva[s]) hva[s] = v;
				if (v < lva[s]) lva[s] = v;
			}
		}
	}
}

void CP2R::SendComment(int pos, int x, int i) {
	CString st, com;
	int current_severity = -1;
	// Clear
	comment[pos + i][vi].clear();
	ccolor[pos + i][vi].clear();
	color[pos + i][vi] = color_noflag;
	if (flag[v][x].size() > 0) for (int f = 0; f < flag[v][x].size(); ++f) {
		GetFlag(f);
		// Send comments and color only if rule is not ignored
		if (accept[sp][vc][vp][fl] == -1 && !show_ignored_flags) continue;
		// Send comments and color only if rule is not ignored
		if (accept[sp][vc][vp][fl] == 1 && !show_allowed_flags) continue;
		// Do not send if ignored
		if (severity[sp][vc][vp][fl] < show_min_severity) continue;
		if (!i) {
			if (!accept[sp][vc][vp][fl]) st = "- ";
			else if (accept[sp][vc][vp][fl] == -1) st = "$ ";
			else st = "+ ";
			com = st + GetRuleName(fl, sp, vc, vp) + " (" + GetSubRuleName(fl, sp, vc, vp) + ")";
			if (show_severity) {
				st.Format(" [%d/%d]", fl, severity[sp][vc][vp][fl]);
				com += st;
			}
			if (GetRuleComment(fl, sp, vc, vp) != "") com += ". " + GetRuleComment(fl, sp, vc, vp);
			if (GetSubRuleComment(fl, sp, vc, vp) != "") com += " (" + GetSubRuleComment(fl, sp, vc, vp) + ")";
			//com += ". ";
			comment[pos][v].push_back(com);
			ccolor[pos][v].push_back(sev_color[severity[sp][vc][vp][fl]]);
		}
		// Set note color if this is maximum flag severity
		if (severity[sp][vc][vp][fl] > current_severity && severity[sp][vc][vp][fl] >= show_min_severity
			&& ruleinfo[fl].viz != vHarm) {
			current_severity = severity[sp][vc][vp][fl];
			color[pos + i][v] = sev_color[severity[sp][vc][vp][fl]];
		}
	}
}

void CP2R::SendCP() {
	CString st;
	//CreateLinks();
	int real_len = cc[0].size();
	int full_len = floor((real_len + 1) / 8 + 1) * 8;
	ResizeVectors(step0 + full_len);
	for (v = 0; v < av_cnt; ++v) {
		vi = vid[v];
		for (ls = 0; ls < fli_size[v]; ++ls) {
			for (s = fli[v][ls]; s <= fli2[v][ls]; ++s) {
				if (cc[v][s]) {
					note[step0 + s][vi] = cc[v][s];
					pause[step0 + s][vi] = 0;
				}
				else {
					note[step0 + s][vi] = 0;
					pause[step0 + s][vi] = 1;
				}
				if (v % 2) {
					lining[step0 + s][v] = HatchStyleDiagonalCross;
				}
				len[step0 + s][vi] = llen[v][ls];
				coff[step0 + s][vi] = s - fli[v][ls];
				tempo[step0 + s] = cp_tempo;
				SendComment(step0 + fli[v][ls], s, s - fli[v][ls]);
			}
		}
		MergeNotes(step0, step0 + full_len - 1);
		st.Format("#%d (from %s)",
			cp_id + 1, bname_from_path(musicxml_file));
		AddMelody(step0, step0 + full_len - 1, vi, st);
	}
	for (int s = step0 + real_len; s < step0 + full_len; ++s) tempo[s] = tempo[s - 1];
	CountOff(step0, step0 + full_len - 1);
	CountTime(step0, step0 + full_len - 1);
	UpdateNoteMinMax(step0, step0 + full_len - 1);
	UpdateTempoMinMax(step0, step0 + full_len - 1);
	t_generated = step0 + full_len - 1;
	Adapt(step0, t_generated);
	t_sent = t_generated;
}

inline void CP2R::ClearReady() {
	fill(data_ready.begin(), data_ready.end(), 0);
}

inline void CP2R::SetReady(int id) {
	data_ready[id] = 1;
}

inline void CP2R::SetReady(int id, int id2) {
	data_ready[id] = 1;
	data_ready[id2] = 1;
}

inline void CP2R::SetReady(int id, int id2, int id3) {
	data_ready[id] = 1;
	data_ready[id2] = 1;
	data_ready[id3] = 1;
}

inline void CP2R::ClearReadyPersist(int id) {
	data_ready_persist[id] = 0;
}

inline void CP2R::ClearReadyPersist(int id, int id2) {
	data_ready_persist[id] = 0;
	data_ready_persist[id2] = 0;
}

inline void CP2R::ClearReadyPersist(int id, int id2, int id3) {
	data_ready_persist[id] = 0;
	data_ready_persist[id2] = 0;
	data_ready_persist[id3] = 0;
}

inline void CP2R::SetReadyPersist(int id) {
	data_ready_persist[id] = 1;
}

inline void CP2R::SetReadyPersist(int id, int id2) {
	data_ready_persist[id] = 1;
	data_ready_persist[id2] = 1;
}

inline void CP2R::SetReadyPersist(int id, int id2, int id3) {
	data_ready_persist[id] = 1;
	data_ready_persist[id2] = 1;
	data_ready_persist[id3] = 1;
}

inline void CP2R::CheckReady(int id) {
	if (!data_ready[id] && !warn_data_ready[id]) {
		++warn_data_ready[id];
		CString est;
		est.Format("Attemp to use data element '%d' while it is not ready yet", id);
		WriteLog(5, est);
		ASSERT(0);
	}
}

inline void CP2R::CheckReady(int id, int id2) {
	CheckReady(id);
	CheckReady(id2);
}

inline void CP2R::CheckReady(int id, int id2, int id3) {
	CheckReady(id);
	CheckReady(id2);
	CheckReady(id3);
}

inline void CP2R::CheckReadyPersist(int id) {
	if (!data_ready_persist[id] && !warn_data_ready_persist[id]) {
		++warn_data_ready_persist[id];
		CString est;
		est.Format("Attemp to use persistent data element '%d' while it is not ready yet", id);
		WriteLog(5, est);
		ASSERT(0);
	}
}

inline void CP2R::CheckReadyPersist(int id, int id2) {
	CheckReadyPersist(id);
	CheckReadyPersist(id2);
}

inline void CP2R::CheckReadyPersist(int id, int id2, int id3) {
	CheckReadyPersist(id);
	CheckReadyPersist(id2);
	CheckReadyPersist(id3);
}

void CP2R::AnalyseCP() {
	skip_flags = 0;
	EvaluateCP();
}

int CP2R::EvaluateCP() {
	CLEAR_READY();
	ClearFlags(0, c_len);
	GetDiatonic(0, c_len);
	GetPitchClass(0, c_len);
	CreateLinks();
	GetNoteTypes();
	GetVca();
	GetLClimax();
	GetLeapSmooth();
	if (av_cnt == 1) {
		FailFirstNotes();
		FailLastNotes();
	}
	for (v = 0; v < av_cnt; ++v) {
		sp = vsp[v];
		vaccept = &accept[sp][1][0];
		if (FailManyLeaps(max_leaps[sp][av_cnt][0], max_leaped[sp][av_cnt][0], max_leaps_r[sp][av_cnt][0],
			max_leaped_r[sp][av_cnt][0], max_leap_steps[sp][av_cnt][0],
			493, 494, 495, 496)) return 1;
		if (FailManyLeaps(max_leaps2[sp][av_cnt][0], max_leaped2[sp][av_cnt][0], max_leaps2_r[sp][av_cnt][0],
			max_leaped2_r[sp][av_cnt][0], max_leap_steps2[sp][av_cnt][0],
			497, 498, 499, 500)) return 1;
		if (FailLeapSmooth(max_smooth2[sp][av_cnt][0], max_smooth_direct2[sp][av_cnt][0],
			cse_leaps[sp][av_cnt][0], cse_leaps_r[sp][av_cnt][0], 302, 303, 501, 502, 1)) return 1;
		FailIntervals();
		if (mminor) {
			if (FailMinor()) return 1;
			if (FailGisTrail()) return 1;
			if (FailFisTrail()) return 1;
		}
		GetBasicMsh();
		ApplyFixedPat();
		if (mminor) {
			if (FailMinorStepwise()) return 1;
		}
		GetDtp();
		if (FailLeap()) return 1;
	}
	return 0;
}

int CP2R::FailManyLeaps(int mleaps, int mleaped, int mleaps2, int mleaped2, int mleapsteps,
	int flag1, int flag2, int flag3, int flag4) {
	CHECK_READY(DR_fli, DR_c);
	int leap_sum = 0;
	int leaped_sum = 0;
	int leap_sum_i = 0;
	g_leaps[v][fli_size[v] - 1] = 0;
	g_leaped[v][fli_size[v] - 1] = 0;
	int win_leaps = 0;
	int win_leapnotes = 0;
	for (ls = 0; ls < fli_size[v] - 1; ++ls) {
		s = fli2[v][ls];
		s1 = fli2[v][ls + 1];
		// Add new leap
		if (leap[v][s] != 0) {
			++leap_sum;
			leaped_sum += abs(c[v][s] - c[v][s1]) + 1;
		}
		// Subtract old leap
		if ((ls >= mleapsteps) && (leap[v][fli2[v][ls - mleapsteps]] != 0)) {
			leap_sum--;
			leaped_sum -= abs(c[v][fli[v][ls - mleapsteps]] - c[v][fli[v][ls - mleapsteps + 1]]) + 1;
		}
		// Get maximum leap_sum
		if (leap_sum > win_leaps) {
			win_leaps = leap_sum;
			leap_sum_i = ls;
		}
		if (leaped_sum > win_leapnotes) {
			win_leapnotes = leaped_sum;
			leap_sum_i = ls;
		}
		// Record for graph
		g_leaps[v][ls] = leap_sum;
		g_leaped[v][ls] = leaped_sum;
		// Calculate penalty 
		if (leap_sum > mleaps) {
			if (!accept[sp][vc][vp][flag1]) ++fpenalty;
		}
		else if (leap_sum > mleaps2) {
			if (!accept[sp][vc][vp][flag3]) ++fpenalty;
		}
		if (leaped_sum > mleaped) {
			if (!accept[sp][vc][vp][flag2]) ++fpenalty;
		}
		else if (leaped_sum > mleaped2) {
			if (!accept[sp][vc][vp][flag4]) ++fpenalty;
		}
	}
	if (win_leaps > mleaps2)
		FLAGVL(flag3, fli[v][leap_sum_i + 1], fli[v][max(0, leap_sum_i - mleapsteps)]);
	else if (win_leaps > mleaps)
		FLAGVL(flag1, fli[v][leap_sum_i + 1], fli[v][max(0, leap_sum_i - mleapsteps)]);
	if (win_leapnotes > mleaped2)
		FLAGVL(flag4, fli[v][leap_sum_i + 1], fli[v][max(0, leap_sum_i - mleapsteps)]);
	else if (win_leapnotes > mleaped)
		FLAGVL(flag2, fli[v][leap_sum_i + 1], fli[v][max(0, leap_sum_i - mleapsteps)]);
	return 0;
}

void CP2R::ClearFlags(int step1, int step2) {
	for (v = 0; v < av_cnt; ++v) {
		for (s = step1; s < step2; ++s) {
			flag[v][s].clear();
			fsl[v][s].clear();
			fvl[v][s].clear();
		}
	}
	fpenalty = 0;
}

void CP2R::GetPitchClass(int step1, int step2) {
	CHECK_READY(DR_c);
	SET_READY(DR_pc);
	for (int v = 0; v < av_cnt; ++v) {
		for (int s = step1; s < step2; ++s) {
			if (cc[v][s]) {
				pc[v][s] = c[v][s] % 7;
				pcc[v][s] = (cc[v][s] + 12 - bn) % 12;
			}
			else {
				pc[v][s] = -1;
				pcc[v][s] = -1;
			}
		}
	}
}

void CP2R::GetDiatonic(int step1, int step2) {
	SET_READY(DR_c);
	for (int v = 0; v < av_cnt; ++v) {
		for (int s = step1; s < step2; ++s) {
			if (cc[v][s]) {
				c[v][s] = CC_C(cc[v][s], bn, mode);
			}
			else {
				c[v][s] = 0;
			}
		}
	}
}

void CP2R::GetLeapSmooth() {
	CHECK_READY(DR_c);
	SET_READY(DR_leap, DR_slur);
	for (int v = 0; v < av_cnt; ++v) {
		for (int i = 0; i < ep2 - 1; ++i) {
			// Find all leaps
			leap[v][i] = 0;
			smooth[v][i] = 0;
			slur[v][i + 1] = 0;
			if (cc[v][i] == cc[v][i + 1]) slur[v][i + 1] = 1;
			if (!cc[v][i] || !cc[v][i + 1]) continue;
			if (c[v][i + 1] - c[v][i] > 1) leap[v][i] = 1;
			else if (c[v][i + 1] - c[v][i] < -1) leap[v][i] = -1;
			// Find all smooth
			else if (c[v][i + 1] - c[v][i] == 1) smooth[v][i] = 1;
			else if (c[v][i + 1] - c[v][i] == -1) smooth[v][i] = -1;
		}
		leap[v][ep2 - 1] = 0;
		smooth[v][ep2 - 1] = 0;
		slur[v][0] = 0;
	}
}

int CP2R::FailIntervals() {
	CHECK_READY(DR_pc, DR_c, DR_fli);
	for (ls = 0; ls < fli_size[v] - 1; ++ls) {
		s0 = fli[v][ls];
		s = fli2[v][ls];
		s1 = fli2[v][ls + 1];
		// Ignore pauses
		if (!cc[v][s1]) continue;
		if (!cc[v][s]) continue;
		// Leap size prohibit
		if (cc[v][s1] - cc[v][s] == 8) FLAGV(175, s0);
		else if (cc[v][s1] - cc[v][s] == -8) FLAGV(181, s0);
		else if (cc[v][s1] - cc[v][s] == 9) FLAGV(176, s0);
		else if (cc[v][s1] - cc[v][s] == -9) FLAGV(182, s0);
		else if (cc[v][s1] - cc[v][s] == 10) FLAGV(177, s0);
		else if (cc[v][s1] - cc[v][s] == -10) FLAGV(183, s0);
		else if (cc[v][s1] - cc[v][s] == 11) FLAGV(178, s0);
		else if (cc[v][s1] - cc[v][s] == -11) FLAGV(184, s0);
		else if (cc[v][s1] - cc[v][s] == 12) FLAGV(179, s0);
		else if (cc[v][s1] - cc[v][s] == -12) FLAGV(185, s0);
		else if (cc[v][s1] - cc[v][s] > 12) FLAGV(180, s0);
		else if (cc[v][s1] - cc[v][s] < -12) FLAGV(186, s0);
		// Prohibit BB
		if (pcc[v][fli[v][ls + 1]] == 11 && pcc[v][s] == 11) FLAGV(348, s0);
	}
	return 0;
}

void CP2R::GetLClimax() {
	SET_READY(DR_lclimax);
	for (int v = 0; v < av_cnt; ++v) {
		GetMovingMax(cc[v], max(lclimax_notes, lclimax_mea * npm), lclimax[v]);
		GetMovingMax(cc[v], lclimax_mea5[vsp[v]][av_cnt][0] * npm, lclimax2[v]);
	}
}

void CP2R::GetNoteTypes() {
	CHECK_READY(DR_fli);
	SET_READY(DR_beat, DR_sus);
	for (v = 0; v < av_cnt; ++v) {
		int l;
		for (ls = 0; ls < fli_size[v]; ++ls) {
			s = fli[v][ls];
			s2 = fli2[v][ls];
			l = llen[v][ls];
			int sm = s % npm;
			// Get beat
			if (sm == 0) beat[v][ls] = 0;
			else if (sm == 1) beat[v][ls] = 10;
			else if (sm == 2) beat[v][ls] = 3;
			else if (sm == 3) beat[v][ls] = 11;
			else if (sm == 4) beat[v][ls] = 1;
			else if (sm == 5) beat[v][ls] = 12;
			else if (sm == 6) beat[v][ls] = 5;
			else if (sm == 7) beat[v][ls] = 13;
			else if (sm == 8) beat[v][ls] = 2;
			else if (sm == 9) beat[v][ls] = 14;
			else if (sm == 10) beat[v][ls] = 6;
			else if (sm == 11) beat[v][ls] = 15;
			else if (sm == 12) beat[v][ls] = 4;
			else if (sm == 13) beat[v][ls] = 16;
			else if (sm == 14) beat[v][ls] = 7;
			else if (sm == 15) beat[v][ls] = 17;
			// Beats for species 1: 0 0 0 0
			// Beats for species 2: 0 1 0 1
			// Beats for species 3: 0 3 1 5
			// Beats for species 4: 0 1 0 1
			// Beats for species 5: 0 10 3 11 1 12 5 13 2 14 6 15 4 16 7 17
			// Get suspension if cantus note changes during counterpoint note
			sus[v][ls] = 0;
			if (bmli[s] != bmli[s2]) {
				sus[v][ls] = mli[bmli[s] + 1];
			}
			// Build isus
			isus[v][ls] = sus[v][ls] ? sus[v][ls] : fli[v][ls];
		}
	}
}

int CP2R::FailGisTrail() {
	CHECK_READY(DR_fli, DR_pc);
	int gis_trail = 0;
	int _gis_trail_max = gis_trail_max[sp][1][0];
	for (ls = 0; ls < fli_size[v]; ++ls) {
		s = fli[v][ls];
		if (cc[v][s]) {
			if (pcc[v][s] == 11) {
				// Set to maximum on new G# note
				gis_trail = _gis_trail_max;
			}
			else {
				if (pcc[v][s] == 10) {
					// Prohibit G note close to G#
					if (gis_trail) FLAGVL(200, s, fli[v][max(0, ls - _gis_trail_max + gis_trail)]);
				}
			}
		}
		// Decrease if not zero
		if (gis_trail) --gis_trail;
	}
	return 0;
}

int CP2R::FailFisTrail() {
	CHECK_READY(DR_fli, DR_pc);
	int pos1, pos2, found;
	int _fis_gis_max = fis_gis_max[sp][1][0];
	int _fis_g_max = fis_g_max[sp][1][0];
	int _fis_g_max2 = fis_g_max2[sp][1][0];
	for (ls = 0; ls < fli_size[v]; ++ls) {
		s = fli[v][ls];
		if (cc[v][s] && pcc[v][s] == 9) {
			// Find VII#
			pos1 = max(0, ls - _fis_gis_max);
			pos2 = min(fli_size[v] - 1, ls + _fis_gis_max);
			found = 0;
			for (int x = pos1; x <= pos2; ++x) {
				s2 = fli[v][x];
				if (cc[v][s2] && pcc[v][s2] == 11) {
					found = 1;
					break;
				}
			}
			if (!found) {
				// Flag only if full melody analysis or window is not cut
				if (ls + _fis_gis_max <= fli_size[v] - 1 || ep2 == c_len)	FLAGVL(199, s, s);
			}
			// Find VII before
			pos1 = max(0, ls - _fis_g_max);
			for (int x = pos1; x < ls; ++x) {
				s2 = fli[v][x];
				if (cc[v][s2] && pcc[v][s2] == 10) {
					FLAGVL(349, s, s2);
					break;
				}
			}
			// Find VII after
			pos2 = min(fli_size[v] - 1, ls + _fis_g_max2);
			for (int x = ls + 1; x <= pos2; ++x) {
				s2 = fli[v][x];
				if (cc[v][s2] && pcc[v][s2] == 10) {
					FLAGVL(350, s, s);
					break;
				}
			}
		}
	}
	return 0;
}

int CP2R::FailMinor() {
	CHECK_READY(DR_pc, DR_fli);
	for (ls = 1; ls < fli_size[v]; ++ls) {
		s = fli[v][ls];
		s_1 = fli[v][ls - 1];
		// Prohibit minor second up before VII - absorbed
		// Prohibit augmented second up before VII - absorbed
		// Prohibit unaltered VI or VII two steps from altered VI or VII
		if (pcc[v][s] == 11) {
			if (pcc[v][s_1] == 10) FLAGVL(153, s_1, s);
			if (pcc[v][s_1] == 8) FLAGVL(154, s_1, s);
			if (pcc[v][s_1] == 3) {
				if (ls < fli_size[v] - 1) {
					// III-VII#-I downward
					if (pcc[v][fli[v][ls + 1]] == 0 && cc[v][s] - cc[v][s_1] < 0) FLAGVL(432, s_1, fli[v][ls + 1]);
					// III-VII#
					else FLAGVL(157, s_1, s);
				}
				else {
					if (ep2 == c_len) FLAGVL(157, s_1, s);
				}
			}
			if (ls > 1) {
				s_2 = fli[v][ls - 2];
				if (pcc[v][s_2] == 10) FLAGVL(159, s_2, s);
				if (pcc[v][s_2] == 8) FLAGVL(160, s_2, s);
				if (pcc[v][s_2] == 3) FLAGVL(163, s_2, s);
			}
			if (ls < fli_size[v] - 1) {
				s1 = fli[v][ls + 1];
				if (pcc[v][s1] == 10) FLAGVL(153, s1, s);
				if (pcc[v][s1] == 8) FLAGVL(154, s1, s);
				if (pcc[v][s1] == 3) FLAGVL(156, s1, s);
				if (ls < fli_size[v] - 2) {
					s2 = fli[v][ls + 2];
					if (pcc[v][s2] == 10) FLAGVL(159, s2, s);
					if (pcc[v][s2] == 8) FLAGVL(160, s2, s);
					if (pcc[v][s2] == 3) FLAGVL(162, s2, s);
				}
			}
		}
		if (pcc[v][s] == 9) {
			if (pcc[v][s_1] == 8) FLAGVL(152, s_1, s);
			if (pcc[v][s_1] == 3) FLAGVL(155, s_1, s);
			if (ls > 1) {
				s_2 = fli[v][ls - 2];
				if (pcc[v][s_2] == 8) FLAGVL(158, s_2, s);
				if (pcc[v][s_2] == 3) FLAGVL(161, s_2, s);
			}
			if (ls < fli_size[v] - 1) {
				s1 = fli[v][ls + 1];
				if (pcc[v][s1] == 8) FLAGVL(152, s1, s);
				if (pcc[v][s1] == 3) FLAGVL(155, s1, s);
				if (ls < fli_size[v] - 2) {
					s2 = fli[v][ls + 2];
					if (pcc[v][s2] == 8) FLAGVL(158, s2, s);
					if (pcc[v][s2] == 3) FLAGVL(161, s2, s);
				}
			}
		}
		// Prohibit unresolved minor tritone DG# (direct or with inserted note)
	}
	return 0;
}

int CP2R::FailMinorStepwise() {
	CHECK_READY(DR_pc, DR_fli);
	CHECK_READY(DR_msh);
	// For non-border notes only, because border notes have their own rules
	for (ls = 1; ls < fli_size[v] - 1; ++ls) {
		s = fli[v][ls];
		s_1 = fli[v][ls - 1];
		s1 = fli[v][ls + 1];
		// Prohibit harmonic VI# not stepwize ascending
		if ((sp < 2 || msh[v][ls] > 0) && pcc[v][s] == 9 &&
			(c[v][s] - c[v][s_1] != 1 || c[v][s1] - c[v][s] != 1))
			FLAGVL(201, s_1, s1);
		// Prohibit harmonic VII natural not stepwize descending
		if ((sp < 2 || msh[v][ls] > 0) && pcc[v][s] == 10 &&
			(c[v][s] - c[v][s_1] != -1 || c[v][s1] - c[v][s] != -1))
			FLAGVL(202, s_1, s1);
	}
	return 0;
}

// Merge notes of same pitch, that do not have pauses between them. Step2 inclusive
void CP2R::MergeNotes(int step1, int step2) {
	// Start of current note
	int first_pos = step1;
	DWORD col = color[step1][v];
	for (int x = step1 + 1; x <= step2; ++x) {
		// Detect steps that have same pitch and there is no retrigger 
		if (coff[x][v]) {
			// select best color: gray is ignored, then most red is selected
			if (color[x][v] != color_noflag &&
				(col == color_noflag || GetRed(color[x][v]) > GetRed(col))) {
				col = color[x][v];
				// update color of previous steps
				for (int z = first_pos; z < x; ++z) {
					color[z][v] = col;
				}
			}
			coff[x][v] = coff[x - 1][v] + 1;
			// Copy color forward
			color[x][v] = col;
		}
		// New note
		else {
			first_pos = x;
			col = color[x][v];
		}
	}
}

void CP2R::GetBasicMsh() {
	CHECK_READY(DR_c, DR_fli, DR_leap);
	SET_READY(DR_mshb);
	// First note is always downbeat
	mshb[v][0] = pDownbeat;
	// Main calculation
	for (ls = 1; ls < fli_size[v]; ++ls) {
		s = fli[v][ls];
		s2 = fli2[v][ls];
		if (s % npm == 0) mshb[v][ls] = pDownbeat;
		else if (s > 0 && leap[v][s - 1]) mshb[v][ls] = pLeapTo;
		else if (s2 < ep2 - 1 && leap[v][s2]) mshb[v][ls] = pLeapFrom;
		else {
			if (s > 0 && s2 < ep2 - 1 && c[v][s - 1] == c[v][s2 + 1]) mshb[v][ls] = pAux;
			else mshb[v][ls] = pPass;
		}
	}
}

void CP2R::GetDtp() {
	CHECK_READY(DR_fli);
	SET_READY(DR_dtp);
	int pause_dist = 0;
	int pause_dist_s = 0;
	for (ls = fli_size[v] - 1; ls >= 0; --ls) {
		s = fli[v][ls];
		dtp[v][ls] = pause_dist;
		dtp_s[v][ls] = pause_dist_s;
		if (cc[v][s]) {
			++pause_dist;
			pause_dist_s += llen[v][ls];
		}
		else {
			pause_dist = 0;
			pause_dist_s = 0;
		}
	}
}

void CP2R::ApplyFixedPat() {
	CHECK_READY(DR_mshb);
	CHECK_READY(DR_fli);
	SET_READY(DR_msh);
	for (int ls = 0; ls < fli_size[v]; ++ls) msh[v][ls] = mshb[v][ls];
}

void CP2R::CountFillInit(int tail_len, int pre, int &t1, int &t2, int &fill_end) {
	// Create leap tail
	tc.clear();
	if (pre) {
		int pos1 = fleap_start - 1;
		int pos2 = max(fleap_start - tail_len, 0);
		if (c[v][leap_end] > c[v][leap_start]) {
			for (int i = pos1; i >= pos2; --i) {
				if (cc[v][fli[v][i]]) tc.push_back(128 - c[v][fli[v][i]]);
			}
			t1 = 128 - c[v][leap_end];
			t2 = 128 - c[v][leap_start];
		}
		else {
			for (int i = pos1; i >= pos2; --i) {
				if (cc[v][fli[v][i]]) tc.push_back(c[v][fli[v][i]]);
			}
			t1 = c[v][leap_end];
			t2 = c[v][leap_start];
		}
	}
	else {
		int pos1 = fleap_end + 1;
		int pos2 = min(fleap_end + tail_len, fli_size[v] - 1);
		if (c[v][leap_end] > c[v][leap_start]) {
			for (int i = pos1; i <= pos2; ++i) {
				if (cc[v][fli[v][i]]) tc.push_back(c[v][fli[v][i]]);
			}
			t1 = c[v][leap_start];
			t2 = c[v][leap_end];
		}
		else {
			for (int i = pos1; i <= pos2; ++i) {
				if (cc[v][fli[v][i]]) tc.push_back(128 - c[v][fli[v][i]]);
			}
			t1 = 128 - c[v][leap_start];
			t2 = 128 - c[v][leap_end];
		}
	}
	for (int x = t1; x <= t2; ++x) nstat3[x] = 0;
	fill_end = -1;
}

void CP2R::CountFill(int tail_len, int &skips, int &fill_to, int pre, int &fill_to_pre, int &fill_from_pre, int &fill_from, int &deviates, int &dev_count, int leap_prev, int &fill_end, int &fill_goal) {
	// Leap starting and finishing note
	int t1, t2;
	int cur_deviation = 0;
	int dev_state = 0;
	int max_deviation = 0;
	if (accept[sp][1][0][42 + leap_id]) max_deviation = 1;
	if (accept[sp][1][0][120 + leap_id] && !pre) max_deviation = 2;
	CountFillInit(tail_len, pre, t1, t2, fill_end);
	// Detect fill_end
	deviates = 0;
	int dl2 = dev_late2[sp][1][0];
	int dl3 = dev_late3[sp][1][0];
	if (leap_size > 4) {
		dl2 = 1;
		dl3 = 2;
	}
	fill_goal = 0;
	// Deviation state: 0 = before deviation, 1 = in deviation, 2 = after deviation, 3 = multiple deviations
	for (int x = 0; x < tc.size(); ++x) {
		// If deviating, start deviation state and calculate maximum deviation
		if (tc[x] > t2) {
			cur_deviation = tc[x] - t2;
			// Detect long deviation for >5th 2nd
			if (cur_deviation == 1 && x == 0 && leap_size > 4 && fleap_end < fli_size[v] - 1 &&
				rlen[v][fleap_end + 1] > dev2_maxlen[sp][1][0] * 2 && !accept[sp][1][0][386]) break;
			// Detect late deviation
			if (cur_deviation == 1 && x >= dl2 && !accept[sp][1][0][191]) break;
			if (cur_deviation == 2 && x >= dl3 && !accept[sp][1][0][192]) break;
			if (cur_deviation > deviates) {
				// If deviation is unacceptable, break leap compensation
				if (cur_deviation > max_deviation) break;
				// Save deviation for later use if it is acceptable
				deviates = cur_deviation;
			}
			if (dev_state == 0) dev_state = 1;
			else if (dev_state == 2) {
				break;
			}
		}
		// If not deviating, switch deviation state
		else {
			if (dev_state == 1) dev_state = 2;
		}
		// Calculate goal
		fill_goal = max(fill_goal, t2 - tc[x]);
		// Break leap compensation if we are below the border
		if (tc[x] < t1) break;
		// Increment leap compensation window
		fill_end = x;
	}
	// Add middle note as compensation note if leap is compound
	if (leap_mid) {
		// Convert note to tc
		if (pre) {
			if (c[v][leap_end] > c[v][leap_start]) ++nstat3[128 - c[v][leap_mid]];
			else ++nstat3[c[v][leap_mid]];
		}
		else {
			if (c[v][leap_end] > c[v][leap_start]) ++nstat3[c[v][leap_mid]];
			else ++nstat3[128 - c[v][leap_mid]];
		}
	}
	// Calculate fill vector
	for (int x = 0; x <= fill_end; ++x) {
		++nstat3[tc[x]];
	}
	// Get deviations count
	if (dev_state == 0) dev_count = 0;
	else dev_count = 1;

	CountFillSkips(leap_prev, skips, t1, t2);
	CountFillLimits(pre, t1, t2, fill_to, fill_to_pre, fill_from_pre, fill_from);
}

void CP2R::CountFillSkips(int leap_prev, int &skips, int t1, int t2) {
	skips = 0;
	for (int x = t1 + 1; x < t2; ++x) if (!nstat3[x]) {
		++skips;
	}
}

void CP2R::CountFillLimits(int pre, int t1, int t2, int &fill_to, int &fill_to_pre, int &fill_from_pre, int &fill_from) {
	fill_to = leap_size;
	fill_to_pre = fill_to;
	fill_from_pre = fill_to;
	fill_from = leap_size;
	// Search for first compensated note
	for (int i = t2 - 1; i >= t1; --i) {
		if (nstat3[i]) {
			fill_from = t2 - i;
			break;
		}
	}
	for (int i = t1; i <= t2; ++i) {
		if (nstat3[i]) {
			fill_to = i - t1;
			break;
		}
	}
	// Check prepared fill to
	if (!pre && fill_to > 1) {
		int pos;
		if (fill_to == 2) pos = max(0, fleap_start - fill_pre3_notes[sp][1][0]);
		else pos = max(0, fleap_start - fill_pre4_notes[sp][1][0]);
		vector<int> nstat4;
		nstat4.resize(2, 0);
		if (c[v][leap_start] < c[v][leap_end]) {
			for (int x = pos; x < fleap_start; ++x) {
				if (c[v][fli[v][x]] == c[v][leap_start] + 1) nstat4[0] = 1;
				else if (c[v][fli[v][x]] == c[v][leap_start] + 2) nstat4[1] = 1;
			}
		}
		else {
			for (int x = pos; x < fleap_start; ++x) {
				if (c[v][fli[v][x]] == c[v][leap_start] - 1) nstat4[0] = 1;
				else if (c[v][fli[v][x]] == c[v][leap_start] - 2) nstat4[1] = 1;
			}
		}
		fill_to_pre = fill_to;
		if (fill_to == 2) {
			if (nstat4[0]) --fill_to_pre;
		}
		else if (fill_to >= 3) {
			if (nstat4[0]) --fill_to_pre;
			if (nstat4[1]) --fill_to_pre;
		}
	}
	// Check prepared fill from
	if (!pre && fill_from > 1) {
		int pos;
		if (fill_from == 2) pos = max(0, fleap_start - fill_pre3_notes[sp][1][0]);
		else pos = max(0, fleap_start - fill_pre4_notes[sp][1][0]);
		vector<int> nstat4;
		nstat4.resize(2, 0);
		if (c[v][leap_start] < c[v][leap_end]) {
			for (int x = pos; x < fleap_start; ++x) {
				if (c[v][fli[v][x]] == c[v][leap_end] - 1) nstat4[0] = 1;
				else if (c[v][fli[v][x]] == c[v][leap_end] - 2) nstat4[1] = 1;
			}
		}
		else {
			for (int x = pos; x < fleap_start; ++x) {
				if (c[v][fli[v][x]] == c[v][leap_end] + 1) nstat4[0] = 1;
				else if (c[v][fli[v][x]] == c[v][leap_end] + 2) nstat4[1] = 1;
			}
		}
		if (fill_from == 2) {
			if (nstat4[0]) fill_from_pre = 1;
		}
		else if (fill_from == 3) {
			if (nstat4[0] && nstat4[1]) fill_from_pre = 1;
		}
	}
}

void CP2R::FailLeapInit(int &late_leap, int &presecond, int &leap_next, int &leap_prev, int &arpeg, int &overflow) {
	presecond = 0; // If leap has a filled second
	leap_next = 0; // Multiply consecutive leaps
	leap_prev = 0; // Multiply consecutive leaps
	overflow = 0; // Leap back overflow
	arpeg = 0; // Arpeggio 3+3
						 // Check if this leap is 3rd
	leap_start = s; // First step of leap
	leap_end = fli2[v][ls + 1]; // Last step of leap
	leap_mid = 0;
	fleap_start = ls;
	fleap_end = ls + 1;
	// leap_size = 4 for 5th
	leap_size = abs(c[v][leap_end] - c[v][s]);
	// Next is leap?
	if (fleap_end < fli_size[v] - 1) leap_next = leap[v][leap_start] * leap[v][leap_end];
	// Prev is leap?
	if (fleap_start > 0) leap_prev = leap[v][leap_start] * leap[v][fli2[v][fleap_start] - 1];
	// Late leap?
	late_leap = dtp_s[v][fleap_start] <= c4p_last_steps || dtp[v][fleap_start] <= c4p_last_notes[sp][1][0];
}

int CP2R::FailLeapMulti(int leap_next, int &arpeg, int &overflow, int &child_leap) {
	child_leap = 0; // If we have a child_leap
									// Check if leap is third
	if (leap_size == 2) {
		// Check if leap is second third
		if (fleap_start > 0 && abs(c[v][leap_end] - c[v][fli2[v][fleap_start - 1]]) == 4 &&
			abs(c[v][leap_start] - c[v][fli2[v][fleap_start - 1]]) == 2) {
			// If there is one more third forward (3 x 3rds total)
			if (fleap_end < fli_size[v] - 1 && abs(c[v][fli2[v][fleap_end + 1]] - c[v][fli2[v][fleap_start - 1]]) == 6) {
				FLAGVL(504, fli[v][fleap_start - 1], fli[v][fleap_start + 1]);
			}
			else FLAGVL(503, fli[v][fleap_start - 1], fli[v][fleap_start + 1]);
			// Set middle leap note
			leap_mid = leap_start;
			// Set leap start to first note of first third
			--fleap_start;
			leap_start = fli2[v][fleap_start];
			// Set leap size to be compound
			leap_size = 4;
		}
	}
	leap_id = min(leap_size - 2, 3);
	if (fleap_end < fli_size[v] - 1) {
		// Next leap in same direction
		if (leap_next > 0) {
			// Flag if greater than two thirds
			if (abs(c[v][fli2[v][fleap_end + 1]] - c[v][leap_start]) > 4)
				FLAGVL(505, fli[v][fleap_start], fli[v][bli[v][leap_end] + 1]);
			// Allow if both thirds, without flags (will process next cycle)
			else arpeg = 1;
		}
		// Next leap back
		else if (leap_next < 0) {
			int leap_size2 = abs(c[v][fli2[v][fleap_end + 1]] - c[v][leap_end]);
			// Flag if back leap greater than 6th
			if (leap_size2 > 5) FLAGV(22, fli[v][bli[v][leap_end]]);
			// Flag if back leap equal or smaller than 6th
			else FLAGV(8, fli[v][bli[v][leap_end]]);
			// Flag leap back overflow
			if (leap_size2 > leap_size) {
				FLAGV(58, fli[v][bli[v][leap_end]]);
				overflow = 1;
			}
		}
	}
	// Check if we have a greater opposite neighbouring leap
	if ((fleap_end < fli_size[v] - 1 && abs(c[v][fli2[v][fleap_end + 1]] - c[v][leap_end]) >= leap_size - 1 && leap[v][leap_start] * leap[v][leap_end]<0) ||
		(fleap_start > 0 && abs(c[v][leap_start] - c[v][fli2[v][fleap_start - 1]]) > leap_size && leap[v][leap_start] * leap[v][fli2[v][fleap_start - 1]]<0)) {
		// Set that we are preleaped (even if we are postleaped)
		child_leap = 1;
	}
	return 0;
}

int CP2R::FailLeap() {
	CHECK_READY(DR_leap, DR_c, DR_fli);
	CHECK_READY(DR_dtp);
	if (sp > 1) {
		CHECK_READY(DR_beat);
	}
	// Get leap size, start, end
	// Check if leap is compensated (without violating compensation rules)
	// If leap is not compensated, check uncompensated rules
	// If uncompensated rules not allowed, flag compensation problems detected (3rd, etc.)
	int child_leap, leap_next, leap_prev, presecond;
	int overflow, arpeg, late_leap;
	// Calculate last steps that are allowed to have C4P
	c4p_last_steps = c4p_last_meas[sp][1][0] * npm;
	// Find late leap border 
	c4p_last_notes2 = min(c4p_last_notes[sp][1][0], fli_size[v] - bli[v][max(0, ep2 - c4p_last_steps)]);
	for (s = 0; s < ep2 - 1; ++s) {
		if (leap[v][s] != 0) {
			ls = bli[v][s];
			FailLeapInit(late_leap, presecond, leap_next, leap_prev,
				arpeg, overflow);
			if (FailLeapMulti(leap_next, arpeg, overflow, child_leap)) return 1;
			// If leap back overflow or arpeggio, do not check leap compensation, because compensating next leap will be enough
			if (!overflow && !arpeg)
				if (FailLeapFill(late_leap, leap_prev, child_leap)) return 1;
			if (!arpeg) if (FailLeapMDC()) return 1;
		}
	}
	return 0;
}

int CP2R::FailLeapFill(int late_leap, int leap_prev, int child_leap) {
	// Prefill parameters
	int ptail_len, pfill_to, pfill_to_pre, pfill_from_pre, pfill_from, pdeviates, pfill_end, pdev_count, pfill_goal;
	// Fill parameters
	int tail_len, fill_to, fill_to_pre, fill_from_pre, fill_from, deviates, fill_end, dev_count, fill_goal;
	filled = 1;
	prefilled = 0;
	int pskips = 10;
	int skips = 10;
	// Calculate allowed skips 
	int allowed_skips = 1;
	if (leap_size > 4) ++allowed_skips;
	if (leap_size > 6) ++allowed_skips;
	if (late_leap) allowed_skips += 2;
	int allowed_pskips = 1;
	if (leap_size > 4) ++allowed_pskips;
	if (leap_size > 6) ++allowed_pskips;
	if (late_leap) allowed_pskips += 1;
	// Check if leap is filled
	tail_len = 2 + (leap_size - 1) * fill_steps_mul;
	// Do not check fill if search window is cut by end of current not-last scan window
	if ((fleap_end + tail_len < fli_size[v]) || (c_len == ep2)) {
		// Check fill only if enough length (checked second time in case of slurs)
		CountFill(tail_len, skips, fill_to, 0, fill_to_pre, fill_from_pre,
			fill_from, deviates, dev_count, leap_prev, fill_end, fill_goal);
		if (skips > allowed_skips) filled = 0;
		else if (fill_to >= 3 && fill_to < fill_pre4_int[sp][1][0] &&
			(fill_to_pre == fill_to || !late_leap ||
				!accept[sp][1][0][144 + leap_id] || (fleap_end < fli_size[v] - 1 && !fill_goal))) filled = 0;
		else if (fill_to > 3 && !late_leap) filled = 0;
		else if (fill_to >= fill_pre4_int[sp][1][0] && late_leap) filled = 0;
		else if (fill_to == 2 && (fill_to_pre < 2 || !fleap_start) && !accept[sp][1][0][100 + leap_id]) filled = 0;
		else if (fill_to == 2 && fill_to_pre > 1 && fleap_start && !accept[sp][1][0][104 + leap_id]) filled = 0;
		else if (fill_from >= 3 && fill_from < fill_pre4_int[sp][1][0] && (!fill_from_pre || !late_leap || !accept[sp][1][0][144 + leap_id])) filled = 0;
		else if (fill_from > 3 && !late_leap) filled = 0;
		else if (fill_from >= fill_pre4_int[sp][1][0] && late_leap) filled = 0;
		else if (fill_from == 2 && !accept[sp][1][0][53 + leap_id]) filled = 0;
		else if (deviates > 2) filled = 0;
		else if (deviates == 1 && !accept[sp][1][0][42 + leap_id]) filled = 0;
		else if (deviates == 2 && !accept[sp][1][0][120 + leap_id]) filled = 0;
		if (!filled) {
			// If starting 3rd
			if (fleap_start == 0 && leap_size == 2 && accept[sp][1][0][1]) {
				FLAGV(1, fli[v][fleap_start]);
				return 0;
			}
			if (child_leap && accept[sp][1][0][116 + leap_id]) FLAGV(116 + leap_id, fli[v][fleap_start]);
			// Check if  leap is prefilled
			else {
				if (ls > 0) {
					ptail_len = 2 + (leap_size - 1) * fill_steps_mul;
					CountFill(ptail_len, pskips, pfill_to, 1, pfill_to_pre, pfill_from_pre, pfill_from,
						pdeviates, pdev_count, leap_prev, pfill_end, pfill_goal);
					prefilled = 1;
					if (pskips > allowed_pskips) prefilled = 0;
					else if (pfill_to > 2) prefilled = 0;
					else if (pfill_to == 2 && !accept[sp][1][0][104 + leap_id]) prefilled = 0;
					else if (pfill_from > 2) prefilled = 0;
					else if (pfill_from == 2 && !accept[sp][1][0][53 + leap_id]) prefilled = 0;
					else if (pdeviates > 2) prefilled = 0;
					else if (pdeviates == 1 && !accept[sp][1][0][42 + leap_id]) prefilled = 0;
					else if (pdeviates == 2 && !accept[sp][1][0][120 + leap_id]) prefilled = 0;
				}
				if (prefilled) {
					if (fli_size[v] - fleap_start <= pre_last_leaps[sp][1][0] + 1) FLAGV(204 + leap_id, fli[v][fleap_start]);
					else FLAGV(112 + leap_id, fli[v][fleap_start]);
				}
				else
					FLAGV(124 + leap_id, fli[v][fleap_start]);
			}
		}
		// Show compensation flags only if successfully compensated
		// This means that compensation errors are not shown if uncompensated (successfully or not)
		else {
			// Flag late uncompensated precompensated leap
			if (fill_to >= 3 && fill_to < fill_pre4_int[sp][1][0] && late_leap)
				FLAGV(144 + leap_id, fli[v][fleap_start]);
			else if (fill_from >= 3 && fill_from < fill_pre4_int[sp][1][0] && late_leap)
				FLAGV(144 + leap_id, fli[v][fleap_start]);
			// Flag prepared unfinished fill if it is not blocking 
			else if (fill_to == 2 && (fill_to_pre < 2 || !fleap_start)) FLAGV(100 + leap_id, fli[v][fleap_start]);
			// Flag unfinished fill if it is not blocking
			else if (fill_to == 2 && fill_to_pre > 1) FLAGV(104 + leap_id, fli[v][fleap_start]);
			// Flag after 3rd if it is not blocking
			if (fill_from == 2) FLAGV(53 + leap_id, fli[v][fleap_start]);
			// Flag deviation if it is not blocking
			if (deviates == 1) FLAGV(42 + leap_id, fli[v][fleap_start]);
			// Flag deviation if it is not blocking
			if (deviates == 2) FLAGV(120 + leap_id, fli[v][fleap_start]);
		}
	}
	return 0;
}

int CP2R::FailLeapMDC() {
	// Melody direction change (MDC)
	// 0 = close, 1 = far, 2 = no
	// Default mdc is close, because beginning equals to close mdc
	mdc1 = 0;
	int prev_note = cc[v][leap_start];
	for (int pos = leap_start - 1; pos >= 0; --pos) {
		if (cc[v][pos] != prev_note) {
			// Check if direction changes or pause or long without changes
			if (!cc[v][pos] || leap[v][leap_start] * (cc[v][pos] - prev_note) > 0 || mdc1 > 1) break;
			prev_note = cc[v][pos];
			++mdc1;
		}
	}
	mdc2 = 0;
	prev_note = cc[v][leap_end];
	for (int pos = leap_end + 1; pos < ep2; ++pos) {
		if (cc[v][pos] != prev_note) {
			// Check if direction changes or pause or long without changes
			if (!cc[v][pos] || leap[v][leap_start] * (cc[v][pos] - prev_note) < 0 || mdc2 > 2) break;
			prev_note = cc[v][pos];
			++mdc2;
		}
	}
	// Do not flag close+close
	if (mdc1 == 0 && mdc2 == 0) return 0;
	// Do not flag last 3rd in SAS, because it can be later converted to 5th
	if (fleap_end == fli_size[v] - 1 && ep2 < c_len && !leap_id) return 0;
	// Close + next
	if (!mdc1 && mdc2 == 1) {
		// Close + aux
		if ((sp == 3 || sp == 5) && (
			(fleap_end >= fli_size[v] - 3 && ep2 < c_len) ||
			(fleap_end < fli_size[v] - 3 && cc[v][fli[v][fleap_end + 2]] == cc[v][leap_end] &&
			(cc[v][fli[v][fleap_end + 3]] - cc[v][fli[v][fleap_end + 2]]) * leap[v][leap_start] < 0)))
			FLAGV(510 + leap_id, fli[v][fleap_start]);
		else FLAGV(128 + leap_id, fli[v][fleap_start]);
	}
	// Close + far
	else if (!mdc1 && mdc2 == 2) FLAGV(140 + leap_id, fli[v][fleap_start]);
	// Close + no
	else if (!mdc1 && mdc2 == 3) FLAGV(108 + leap_id, fli[v][fleap_start]);
	// next + close
	else if (mdc1 == 1 && !mdc2) {
		if (sp < 2 || bmli[fli[v][fleap_end]] == bmli[leap_start]) {
			// Aux + close
			if ((sp == 3 || sp == 5) && fleap_start > 2 && cc[v][fli[v][fleap_start - 2]] == cc[v][leap_start] &&
				(cc[v][fli[v][fleap_start - 2]] - cc[v][fli[v][fleap_start - 3]]) * leap[v][leap_start] < 0)
				FLAGV(506 + leap_id, fli[v][fleap_start]);
			else FLAGV(59 + leap_id, fli[v][fleap_start]);
		}
		else FLAGV(476 + leap_id, fli[v][fleap_start]);
	}
	// Far + close
	else if (mdc1 == 2 && !mdc2) {
		if (sp < 2 || bmli[fli[v][fleap_end]] == bmli[leap_start]) FLAGV(132 + leap_id, fli[v][fleap_start]);
		else FLAGV(25 + leap_id, fli[v][fleap_start]);
	}
	// Next + next
	else if (mdc1 == 1 && mdc2 == 1) {
		if (sp < 2 || bmli[fli[v][fleap_end]] == bmli[leap_start]) {
			// Aux + aux
			if ((sp == 3 || sp == 5) && (
				(fleap_end >= fli_size[v] - 3 && ep2 < c_len) || (fleap_end < fli_size[v] - 3 && cc[v][fli[v][fleap_end + 2]] == cc[v][leap_end] &&
				(cc[v][fli[v][fleap_end + 3]] - cc[v][fli[v][fleap_end + 2]]) * leap[v][leap_start] < 0)) &&
				fleap_start > 2 && cc[v][fli[v][fleap_start - 2]] == cc[v][leap_start] &&
				(cc[v][fli[v][fleap_start - 2]] - cc[v][fli[v][fleap_start - 3]]) * leap[v][leap_start] < 0)
				FLAGV(414 + leap_id, fli[v][fleap_start]);
			else FLAGV(63 + leap_id, fli[v][fleap_start]);
		}
		else FLAGV(460 + leap_id, fli[v][fleap_start]);
	}
	// Next + far
	else if (mdc1 == 1 && mdc2 >= 2) {
		if (sp < 2 || bmli[fli[v][fleap_end]] == bmli[leap_start]) FLAGV(391 + leap_id, fli[v][fleap_start]);
		else FLAGV(464 + leap_id, fli[v][fleap_start]);
	}
	// Far + next
	else if (mdc1 >= 2 && mdc2 == 1) {
		if (sp < 2 || bmli[fli[v][fleap_end]] == bmli[leap_start]) FLAGV(148 + leap_id, fli[v][fleap_start]);
		else FLAGV(468 + leap_id, fli[v][fleap_start]);
	}
	// Far + far
	else if (mdc1 >= 2 && mdc2 >= 2) {
		if (sp < 2 || bmli[fli[v][fleap_end]] == bmli[leap_start]) FLAGV(398 + leap_id, fli[v][fleap_start]);
		else FLAGV(472 + leap_id, fli[v][fleap_start]);
	}
	return 0;
}

// Count limits
void CP2R::GetMelodyInterval(int step1, int step2) {
	SET_READY(DR_nmin);
	// Calculate range
	nmin[v] = MAX_NOTE;
	nmax[v] = 0;
	for (int i = step1; i < step2; ++i) if (cc[v][i]) {
		if (cc[v][i] < nmin[v]) nmin[v] = cc[v][i];
		if (cc[v][i] > nmax[v]) nmax[v] = cc[v][i];
	}
	// Calculate diatonic limits
	nmind[v] = CC_C(nmin[v], bn, mode);
	nmaxd[v] = CC_C(nmax[v], bn, mode);
}

void CP2R::ValidateFlags() {
	for (v = 0; v < av_cnt; ++v) {
		for (s = 0; s < c_len; ++s) {
			if (flag[v][s].size()) {
				if (cc[v][s]) {
					// Note start is ok
					if (s == fli[v][bli[v][s]]) continue;
					// Downbeat is ok
					if (!(s % npm)) continue;
				}
				for (int f = 0; f < flag[v][s].size(); ++f) {
					GetFlag(f);
					CString est;
					est.Format("Detected flag at hidden position %d/%d: [%d] %s %s (%s)",
						s, fsl[v][s][f], fl, accept[sp][vc][vp][fl] ? "+" : "-",
						GetRuleName(fl, sp, vc, vp), GetSubRuleName(fl, sp, vc, vp));
					WriteLog(5, est);
				}
			}
		}
	}
}

// Check if too many leaps
int CP2R::FailLeapSmooth(int l_max_smooth, int l_max_smooth_direct, int csel, int csel2,
	int flag1, int flag2, int flag3, int flag4, int first_run) {
	CHECK_READY(DR_leap, DR_c, DR_fli);
	// Clear variables
	int leap_sum2 = 0;
	int thirds_sum = 0;
	int leap_sum_corrected = 0;
	int max_leap_sum2 = 0;
	int smooth_sum = 0;
	int smooth_sum2 = 0;
	int leap_sum_s2 = 0;
	int fired4 = 0, fired5 = 0;
	for (ls = 0; ls < fli_size[v] - 1; ++ls) {
		s = fli2[v][ls];
		s1 = fli2[v][ls + 1];
		// Add new leap
		if (leap[v][s] != 0) {
			++leap_sum2;
			if (abs(c[v][s1] - c[v][s]) == 2) ++thirds_sum;
		}
		else {
			leap_sum2 = 0;
			thirds_sum = 0;
		}
		// Get maximum leap_sum
		leap_sum_corrected = leap_sum2 - min(thirds_sum, thirds_ignored[sp][1][0]);
		if (leap_sum_corrected > max_leap_sum2) {
			max_leap_sum2 = leap_sum_corrected;
			leap_sum_s2 = s;
		}
		// Calculate penalty
		if (leap_sum_corrected == csel) {
			if (accept[sp][1][0][flag3] > 0) ++fpenalty;
		}
		if (leap_sum_corrected > csel2 && accept[sp][1][0][flag4] > 0) ++fpenalty;
		// Prohibit long smooth movement
		if (smooth[v][s] != 0) {
			++smooth_sum;
			if (smooth_sum >= l_max_smooth) {
				if (fired4) {
					fpenalty += severity[sp][1][0][flag1] + 1;
				}
				else {
					FLAGVL(flag1, fli[v][ls + 1], fli[v][ls - smooth_sum + 1]);
					fired4 = 1;
				}
			}
		}
		else if (leap[v][s]) smooth_sum = 0;
		if (ls < fli_size[v] - 2) {
			// Prohibit long smooth movement in one direction
			if (smooth[v][s] != 0 && smooth[v][s] == smooth[v][fli2[v][ls + 1]]) {
				++smooth_sum2;
				if (smooth_sum2 >= l_max_smooth_direct) {
					if (fired5) {
						fpenalty += severity[sp][1][0][flag2] + 1;
					}
					else {
						FLAGVL(flag2, fli[v][ls + 1], fli[v][ls - smooth_sum2 + 1]);
						fired5 = 1;
					}
				}
			}
			else if (smooth[v][s] || leap[v][s]) smooth_sum2 = 0;
			// Check if two notes repeat (including pause)
			if (first_run && ls > 0 && (cc[v][s] == cc[v][fli2[v][ls + 2]]) &&
				(cc[v][fli2[v][ls - 1]] == cc[v][fli2[v][ls + 1]]) &&
				(ep2 == c_len || ls + 2 < fli_size[v] - 1)) {
				// For cantus or species 1 / 4 do not check measure
				if (sp == 0 || sp == 1 || sp == 4) {
					// Same rhythm in first notes of repeat?
					if (llen[v][ls - 1] == llen[v][ls + 1]) {
						if (llen[v][ls - 1] == llen[v][ls]) {
							FLAGVL(402, fli[v][ls - 1], fli[v][ls + 2]);
						}
						else FLAGVL(403, fli[v][ls - 1], fli[v][ls + 2]);
					}
					else FLAGVL(404, fli[v][ls - 1], fli[v][ls + 2]);
				}
				// For species 2 / 3 / 5 check measure
				else {
					// Inside measure?
					if (bmli[fli[v][ls - 1]] == bmli[fli[v][ls + 2]]) {
						// Same rhythm in first notes of repeat?
						if (llen[v][ls - 1] == llen[v][ls + 1]) {
							if (llen[v][ls - 1] == llen[v][ls]) {
								FLAGVL(411, fli[v][ls - 1], fli[v][ls + 2]);
							}
							else FLAGVL(412, fli[v][ls - 1], fli[v][ls + 2]);
						}
						else FLAGVL(413, fli[v][ls - 1], fli[v][ls + 2]);
					}
					else {
						// Same rhythm in first notes of repeat?
						if (llen[v][ls - 1] == llen[v][ls + 1]) {
							if (llen[v][ls - 1] == llen[v][ls]) {
								FLAGVL(402, fli[v][ls - 1], fli[v][ls + 2]);
							}
							else FLAGVL(403, fli[v][ls - 1], fli[v][ls + 2]);
						}
						else FLAGVL(404, fli[v][ls - 1], fli[v][ls + 2]);
					}
				}
			}
		}
	}
	if (first_run && max_leap_sum2 >= csel) {
		if (max_leap_sum2 > csel2)
			FLAGVL(flag4, fli[v][bli[v][leap_sum_s2] + 1], fli[v][max(0, bli[v][leap_sum_s2] - max_leap_sum2 + 1)]);
		else FLAGVL(flag3, fli[v][bli[v][leap_sum_s2] + 1], fli[v][max(0, bli[v][leap_sum_s2] - max_leap_sum2 + 1)]);
	}
	return 0;
}

int CP2R::FailStagnation(int steps, int notes, int fl) {
	CHECK_READY(DR_nmin, DR_fli);
	// Do not test if flag disabled and not evaluating
	if (task != tEval && accept[sp][1][0][fl] == -1) return 0;
	// Clear nstat
	for (int i = nmin[v]; i <= nmax[v]; ++i) nstat[i] = 0;
	for (ls = 0; ls < fli_size[v]; ++ls) {
		s = fli[v][ls];
		// Add new note to stagnation array
		++nstat[cc[v][s]];
		// Subtract old note
		if (ls >= steps) --nstat[cc[v][fli[v][ls - steps]]];
		// Check if too many repeating notes
		if (nstat[cc[v][s]] > notes) FLAGVL(fl, s, fli[v][max(0, ls - steps)]);
	}
	return 0;
}

// Prohibit multiple culminations
int CP2R::FailMultiCulm() {
	CHECK_READY(DR_fli);
	int culm_count = 0;
	int culm_ls = -1;
	int multi_culm_fired = 0;
	// Do not find culminations if too early
	if (ep2 < c_len) return 0;
	for (ls = 0; ls < fli_size[v]; ++ls) {
		if (cc[v][fli[v][ls]] == nmax[v]) {
			++culm_count;
			culm_ls = ls;
			if (culm_count > 1) {
				if (v == av_cnt - 1 && av_cnt > 1) {
					if (multi_culm_fired) fpenalty += severity[sp][1][0][12] + 1;
					else {
						multi_culm_fired = 1;
						FLAGV(12, fli[v][culm_ls]);
					}
				}
				else {
					if (multi_culm_fired) fpenalty += severity[sp][1][0][305] + 1;
					else {
						multi_culm_fired = 1;
						FLAGV(305, fli[v][culm_ls]);
					}
				}
			}
		}
	}
	if (culm_ls == -1) {
		culm_ls = 0;
		CString est;
		est.Format("Warning: culm_ls cannot be detected");
		WriteLog(5, est);
	}
	if (v == av_cnt - 1) {
		// Prohibit culminations at first steps
		if (culm_ls < (early_culm3[sp][1][0] * fli_size[v]) / 100) FLAGV(193, fli[v][culm_ls]);
		if (culm_ls < early_culm[sp][1][0] - 1) FLAGV(78, fli[v][culm_ls]);
		else if (culm_ls < early_culm2[sp][1][0] - 1) FLAGV(79, fli[v][culm_ls]);
		// Prohibit culminations at last steps
		if (culm_ls >= fli_size[v] - late_culm[sp][1][0]) FLAGV(21, fli[v][culm_ls]);
	}
	return 0;
}

int CP2R::FailFirstNotes() {
	CHECK_READY(DR_fli, DR_pc);
	// Prohibit first note not tonic
	if (pc[v][0] != 0) {
		FLAGV(33, 0);
	}
	return 0;
}

int CP2R::FailLastNotes() {
	CHECK_READY(DR_fli, DR_pc);
	// Do not check if melody is short yet
	if (fli_size[v] < 3) return 0;
	// Prohibit last note not tonic
	if (ep2 == c_len) {
		s = fli[v][fli_size[v] - 1];
		s_1 = fli[v][fli_size[v] - 2];
		s_2 = fli[v][fli_size[v] - 3];
		if (pc[v][s] != 0) FLAGV(50, s);
		if (mode == 9) {
			// Prohibit major second up before I
			if (pcc[v][s] == 0 && pcc[v][s_1] == 10) FLAGV(74, s_1, s);
			if (pcc[v][s] == 0 && pcc[v][s_2] == 10) FLAGV(74, s_2, s);
		}
	}
	// Wrong second to last note (last note never can be slurred)
	if (ep2 > c_len - 2) {
		s_1 = c_len - 2;
		if ((pc[v][s_1] == 0) || (pc[v][s_1] == 2) || (pc[v][s_1] == 3) || (pc[v][s_1] == 5)) FLAGV(13, s_1);
		if (pc[v][s_1] == 4) FLAGV(51, s_1);
	}
	// Wrong third to last note
	if (ep2 > c_len - 3) {
		s_2 = c_len - 3;
		if ((pc[v][s_2] == 0) || (pc[v][s_2] == 2) || (pc[v][s_2] == 4)) FLAGV(14, s_2);
		// Leading third to last note
		if (pc[v][s_2] == 6) FLAGV(34, s_2);
	}
	return 0;
}

// Search for adjacent or symmetric repeats
int CP2R::FailAdSymRepeat(int relen) {
	CHECK_READY(DR_fli, DR_c);
	// Do not test if flag disabled and not testing
	//if (task != tEval && accept[sp][1][0][flag] == -1) return 0;
	// Cycle through all notes that can be repeated
	for (ls = 0; ls <= fli_size[v] - relen * 2; ++ls) {
		int rpos1 = 0;
		int rpos2 = 0;
		// Check adjacent repeat
		if (IsRepeat(ls, ls + relen, relen)) {
			rpos1 = ls + relen;
		}
		// If same beat is not adjacent, get same beat and check
		else if (sp > 1 && ((fli[v][ls + relen] - fli[v][ls]) % (npm / 2))) {
			for (int x = 1; x < 4; ++x) {
				if (ls + x <= fli_size[v] - relen * 2 && !((fli[v][ls + relen + x] - fli[v][ls]) % (npm / 2))) {
					if (IsRepeat(ls, ls + relen + x, relen)) {
						rpos1 = ls + relen + x;
					}
				}
			}
		}
		// If any repeat is found, check one more repeat
		if (rpos1 && rpos1 <= fli_size[v] - relen * 2) {
			// Check adjacent repeat
			if (IsRepeat(ls, rpos1 + relen, relen)) {
				rpos2 = rpos1 + relen;
			}
			// If same beat is not adjacent, get same beat and check
			else if (sp > 1 && ((fli[v][rpos1 + relen] - fli[v][rpos1]) % (npm / 2))) {
				for (int x = 1; x < 4; ++x) {
					if (rpos1 + x <= fli_size[v] - relen * 2 && !((fli[v][rpos1 + relen + x] - fli[v][rpos1]) % (npm / 2))) {
						if (IsRepeat(rpos1, rpos1 + relen + x, relen)) {
							rpos2 = rpos1 + relen + x;
						}
					}
				}
			}
		}
		if (relen == 3) {
			// Flag two repeats
			if (rpos2) {
				if (rpos2 == rpos1 + relen)
					FLAGVL(313, fli[v][ls], fli[v][ls + relen - 1]);
				else
					FLAGVL(407, fli[v][ls], fli[v][ls + relen - 1]);
			}
			// Flag one repeat
			else if (rpos1) {
				if (rpos1 == ls + relen)
					FLAGVL(311, fli[v][ls], fli[v][ls + relen - 1]);
				else
					FLAGVL(405, fli[v][ls], fli[v][ls + relen - 1]);
			}
		}
		if (relen == 4) {
			// Flag two repeats
			if (rpos2) {
				if (rpos2 == rpos1 + relen)
					FLAGVL(314, fli[v][ls], fli[v][ls + relen - 1]);
				else
					FLAGVL(408, fli[v][ls], fli[v][ls + relen - 1]);
			}
			// Flag one repeat
			else if (rpos1) {
				if (rpos1 == ls + relen)
					FLAGVL(312, fli[v][ls], fli[v][ls + relen - 1]);
				else
					FLAGVL(406, fli[v][ls], fli[v][ls + relen - 1]);
			}
		}
	}
	return 0;
}

int CP2R::IsRepeat(int ls1, int ls2, int relen) {
	int found = 1;
	for (int i = 0; i < relen; ++i) {
		if (c[v][fli[v][ls1 + i]] != c[v][fli[v][ls2 + i]]) {
			found = 0;
			break;
		}
		if (i < relen - 1 && llen[v][ls1 + i] != llen[v][ls2 + i]) {
			found = 0;
			break;
		}
	}
	return found;
}

// Moving average
void CP2R::maVector(vector<float> &v, vector<float> &v2, int range) {
	int pos1, pos2;
	float ma, maw_sum;
	for (int s = 0; s < ep2; ++s) {
		pos1 = max(0, s - range);
		pos2 = min(ep2 - 1, s + range);
		ma = 0;
		maw_sum = 0;
		for (int x = pos1; x <= pos2; ++x) if (v[x]) {
			ma += v[x];
			++maw_sum;
		}
		if (maw_sum) v2[s] = ma / maw_sum;
		else v2[s] = 0;
	}
}

void CP2R::maVector(vector<int> &v, vector<float> &v2, int range) {
	int pos1, pos2;
	float ma, maw_sum;
	for (int s = 0; s < ep2; ++s) {
		pos1 = max(0, s - range);
		pos2 = min(ep2 - 1, s + range);
		ma = 0;
		maw_sum = 0;
		for (int x = pos1; x <= pos2; ++x) if (v[x]) {
			ma += v[x];
			++maw_sum;
		}
		if (maw_sum) v2[s] = ma / maw_sum;
		else v2[s] = 0;
	}
}

void CP2R::mawVector(vector<int> &v, vector<float> &v2, int range) {
	int pos1, pos2;
	float ma, maw_sum;
	for (int s = 0; s < ep2; ++s) {
		pos1 = max(0, s - range);
		pos2 = min(ep2 - 1, s + range);
		ma = 0;
		maw_sum = 0;
		for (int x = pos1; x <= pos2; ++x) if (v[x]) {
			ma += maw[abs(x - s)] * v[x];
			maw_sum += maw[abs(x - s)];
		}
		if (maw_sum) v2[s] = ma / maw_sum;
		else v2[s] = 0;
	}
}

void CP2R::mawVector(vector<float> &v, vector<float> &v2, int range) {
	int pos1, pos2;
	float ma, maw_sum;
	for (int s = 0; s < ep2; ++s) {
		pos1 = max(0, s - range);
		pos2 = min(ep2 - 1, s + range);
		ma = 0;
		maw_sum = 0;
		for (int x = pos1; x <= pos2; ++x) if (v[x]) {
			ma += maw[abs(x - s)] * v[x];
			maw_sum += maw[abs(x - s)];
		}
		if (maw_sum) v2[s] = ma / maw_sum;
		else v2[s] = 0;
	}
}

void CP2R::MakeMacc() {
	SET_READY(DR_macc);
	int pos1, pos2;
	int ma_range = 2 * minl[v];
	macc_range = ma_range;
	macc2_range = ma_range * 2;
	// Deviation weight
	maw.clear();
	for (int i = 0; i <= ma_range; ++i) {
		maw.push_back(1 - i * 0.5 / ma_range);
	}
	float de, maw_sum;
	// Moving average
	mawVector(cc[v], macc, ma_range);
	// Smooth
	mawVector(macc, macc2, ma_range);
	// Deviation
	for (int s = 0; s < ep2; ++s) {
		pos1 = max(0, s - ma_range);
		pos2 = min(ep2 - 1, s + ma_range);
		de = 0;
		maw_sum = 0;
		for (int x = pos1; x <= pos2; ++x) if (cc[v][x]) {
			de += maw[abs(x - s)] * SQR(cc[v][x] - macc[s]);
			maw_sum += maw[abs(x - s)];
		}
		if (maw_sum) decc[s] = SQRT(de / maw_sum);
		else decc[s] = 0;
	}
	// Smooth
	mawVector(decc, decc2, ma_range);
}

int CP2R::FailLocalMacc(int notes, float mrange, int fl) {
	CHECK_READY(DR_fli, DR_macc);
	// Do not test if flag disabled and not testing
	if (task != tEval && accept[sp][1][0][fl] == -1) return 0;
	// Do not test if not enough notes. If melody is short, than global range check is enough
	if (fli_size[v] < notes) return 0;
	float lmin, lmax, maccr;
	int maccr_count = 0;
	int s;
	int ls_max = fli_size[v] - notes;
	int ls_max2;
	int fired = 0;
	// Loop through windows
	for (ls = 0; ls <= ls_max; ++ls) {
		lmin = MAX_NOTE;
		lmax = 0;
		ls_max2 = ls + notes;
		// Do not check if later notes are not created
		if (ep2 < c_len && fli2[v][ls_max2 - 1] + macc2_range >= ep2) continue;
		// Loop inside each window
		for (s = fli[v][ls]; s <= fli2[v][ls_max2 - 1]; ++s) {
			if (macc2[s] < lmin) lmin = macc2[s];
			if (macc2[s] > lmax) lmax = macc2[s];
		}
		// Record range
		maccr = lmax - lmin;
		++maccr_count;
		// Check range
		if (lmin < MAX_NOTE && lmax > 0 && maccr < mrange) {
			if (fired) {
				fpenalty += severity[sp][1][0][fl] + 1;
			}
			else {
				FLAGVL(fl, fli[v][ls], fli[v][ls_max2 - 1]);
				fired = 1;
			}
		}
	}
	return 0;
}

int CP2R::FailLocalRange(int notes, int mrange, int fl) {
	CHECK_READY(DR_fli, DR_c);
	// Do not test if flag disabled and not testing
	//if (task != tEval && accept[sp][1][0][fl] == -1) return 0;
	// Do not test if not enough notes. If melody is short, than global range check is enough
	if (fli_size[v] < notes) return 0;
	int lmin, lmax, s;
	int ls_max = fli_size[v] - notes;
	int ls_max2;
	int fired = 0;
	int finish = 0;
	// Loop through windows
	for (ls = 0; ls <= ls_max; ++ls) {
		lmin = MAX_NOTE;
		lmax = 0;
		ls_max2 = ls + notes;
		// Loop inside each window
		for (int ls2 = ls; ls2 < ls_max2; ++ls2) {
			s = fli[v][ls2];
			// Process non-pauses
			if (cc[v][s]) {
				if (c[v][s] < lmin) lmin = c[v][s];
				if (c[v][s] > lmax) lmax = c[v][s];
			}
			else {
				// If we see pause, grow window if possible by two notes (one for pause and one additionally, because pause makes stagnation less standing out
				if (ls_max2 < fli_size[v] - 1) ls_max2 += 2;
				// If not possible, finish scanning
				else {
					finish = 1;
				}
			}
		}
		// Finish if we faced pause at the end
		if (finish) break;
		// Check range
		if (lmax - lmin < mrange) {
			if (fired) {
				fpenalty += severity[sp][1][0][fl] + 1;
			}
			else {
				FLAGVL(fl, fli[v][ls], fli[v][ls_max2 - 1]);
				fired = 1;
			}
		}
	}
	return 0;
}

int CP2R::FailLocalPiCount(int notes, int picount, int fl) {
	CHECK_READY(DR_fli, DR_nmin);
	// Do not test if flag disabled and not testing
	if (task != tEval && accept[sp][1][0][fl] == -1) return 0;
	// Do not test if not enough notes
	if (fli_size[v] < notes) return 0;
	int picount2, i;
	// Clear nstat
	for (i = nmin[v]; i <= nmax[v]; ++i) nstat[i] = 0;
	int pauses_inside = 0;
	for (ls = 0; ls < fli_size[v]; ++ls) {
		s = fli[v][ls];
		// Add new note to stagnation array
		++nstat[cc[v][s]];
		// Subtract old note
		if (ls >= notes) {
			--nstat[cc[v][fli[v][ls - notes]]];
		}
		// Check if little pitches
		if (ls >= notes - 1) {
			picount2 = 0;
			for (i = nmin[v]; i <= nmax[v]; ++i) if (nstat[i]) ++picount2;
			// Count pause as pitch for short windows
			if (notes < 10) {
				if (nstat[0]) ++picount2;
			}
			// For long windows do nothing - this shortens window a little, but this is not very important
			if (picount2 < picount) FLAGVL(fl, fli[v][ls - notes + 1], fli[v][ls]);
		}
	}
	return 0;
}

float CP2R::GetTonicWeight(int l_ls, int tt) {
	int l_s = fli[v][l_ls];
	// Not species 3 or 5?
	if (sp != 3 && sp != 5) return 1.0;
	// Tonic downbeat?
	if (!beat[v][l_ls]) return 1.0;
	// Length is above quarter (for species 5)?
	if (rlen[v][l_ls] > 2) return 1.0;
	// Length of tonic note is greater than previous note?
	if (l_ls > 0 && llen[v][l_ls] > llen[v][l_ls - 1]) return 1.0;
	// Leap to tonic?
	if (l_ls > 0 && abs(cc[v][l_s] - cc[v][fli[v][l_ls] - 1]) > 4) return 1.0;
	// Local climax?
	if (c[v][l_s] >= lclimax[v][l_s]) return 1.0;
	return 0.9;
}

int CP2R::FailTonic(int tt) {
	CHECK_READY(DR_pc, DR_fli);
	CHECK_READY(DR_lclimax);
	if (sp) {
		CHECK_READY(DR_beat);
	}
	vector<float> tcount;
	int s9;
	tcount.resize(13);
	int fire, fired = 0;
	// Find first and last tonic
	int first_tonic = -1;
	int last_tonic = -1;
	for (ls = 0; ls < fli_size[v]; ++ls) {
		if (!pc[v][fli[v][ls]]) {
			first_tonic = ls;
			break;
		}
	}
	for (ls = fli_size[v] - 1; ls >= 0; --ls) {
		if (!pc[v][fli[v][ls]]) {
			last_tonic = ls;
			break;
		}
	}
	// Do not check if melody is short
	if (fli_size[v] < 3) return 0;
	int tw = tonic_window[tt][sp][1][0];
	int tm = tonic_max[tt][sp][1][0];
	for (ls = 0; ls < fli_size[v]; ++ls) {
		s = fli[v][ls];
		// Decrement for previous tonic note
		if (ls >= tw) {
			s9 = fli[v][ls - tw];
			if (!pc[v][s9])
				tcount[cc[v][s9] / 12] -= GetTonicWeight(ls - tw, tt);
		}
		if (!pc[v][s]) {
			// Increment for current tonic note
			tcount[cc[v][s] / 12] += GetTonicWeight(ls, tt);
			// Check count of tonic notes
			if (tcount[cc[v][s] / 12] >= tm) {
				if (fired) {
					fpenalty += severity[sp][1][0][70 + tt] + 1;
				}
				else {
					FLAGVL(70 + tt, s, fli[v][max(0, ls - tw)]);
					fired = 1;
				}
			}
		}
		tweight[v][ls] = vmax(tcount);
	}
	return 0;
}

int CP2R::FailLastNoteRes() {
	CHECK_READY(DR_pc, DR_fli);
	if (ep2 < c_len) return 0;
	if (fli_size[v] < 2) return 0;
	if (pc[v][fli[v][fli_size[v] - 2]] == 6 && pc[v][c_len - 1] != 0) FLAGV(52, fli[v][fli_size[v] - 2]);
	if (pc[v][fli[v][fli_size[v] - 2]] == 3 && pc[v][c_len - 1] != 2) FLAGV(87, fli[v][fli_size[v] - 2]);
	return 0;
}

// Check tritone t1-t2 which has to resolve from ta to tb. Use fleap_start/fleap_end
void CP2R::GetTritoneResolution(int ta, int t1, int t2, int tb, int &res1, int &res2) {
	res1 = 0;
	res2 = 0;
	// Real resolution notes
	int ta2, tb2;
	// Get real resolution notes
	if (pcc[v][fli[v][fleap_end]] == t2) {
		ta2 = ta;
		tb2 = tb;
		// Outer resolution is not a resolution
		if (cc[v][fli[v][fleap_end]] > cc[v][fli[v][fleap_start]]) return;
	}
	else {
		ta2 = tb;
		tb2 = ta;
		// Outer resolution is not a resolution
		if (cc[v][fli[v][fleap_end]] < cc[v][fli[v][fleap_start]]) return;
	}
	// Get resolution window
	int rwin = 1;
	if (av_cnt > 1 && bmli[fli[v][fleap_start]] >= mli.size() - 2) rwin = max(1, (npm * tritone_res_quart[sp][1][0]) / 4);
	// Scan preparation
	if (fleap_start > 0) {
		int pos1 = max(0, fli[v][fleap_start] - rwin);
		int pos2 = min(ep2, fli[v][fleap_end]);
		for (int i = pos1; i < pos2; ++i) {
			if (pcc[v][i] == ta2 && abs(cc[v][i] - cc[v][fli[v][fleap_start]]) < 5) {
				res1 = 1;
				break;
			}
		}
	}
	// Do not check if cut by scan window
	if (fli2[v][fleap_end] + 1 + rwin > ep2 && ep2 < c_len) {
		res2 = 1;
		return;
	}
	// Scan resolution
	if (fleap_end < fli_size[v] - 1) {
		int pos1 = max(0, fli2[v][fleap_start] + 1);
		int pos2 = min(ep2, fli2[v][fleap_end] + 1 + rwin);
		for (int i = pos1; i < pos2; ++i) {
			if (pcc[v][i] == tb2 && abs(cc[v][i] - cc[v][fli[v][fleap_end]]) < 5) {
				res2 = 1;
				break;
			}
		}
	}
}

// Check tritone t1-t2 which has to resolve from ta to tb
int CP2R::FailTritone(int ta, int t1, int t2, int tb) {
	int found;
	int res1 = 0; // First note resolution flag
	int res2 = 0; // Second note resolution flag
								// Tritone prohibit
	leap_start = s;
	found = 0;
	// Check consecutive tritone
	if ((pcc[v][s1] == t2 && pcc[v][s] == t1) || (pcc[v][s1] == t1 && pcc[v][s] == t2)) found = 1;
	// Check tritone with additional note inside
	else if (ls > 0) {
		// Check pitches
		if ((pcc[v][s1] == t2 && pcc[v][s_1] == t1) || (pcc[v][s1] == t1 && pcc[v][s_1] == t2))
			// Check intermediate note and mdc
			if (c[v][s] > c[v][s1] && c[v][s] < c[v][s_1]) {
				if ((ls < 2 || c[v][s_2] < c[v][s_1] || leap[v][s_2]) && (ls > fli_size[v] - 3 || c[v][s2] > c[v][s1] || leap[v][s1])) {
					found = 2;
					leap_start = s_1;
				}
			}
			else if (c[v][s] < c[v][s1] && c[v][s] > c[v][s_1]) {
				if ((ls<2 || c[v][s_2] > c[v][s_1] || leap[v][s_2]) && (ls>fli_size[v] - 3 || c[v][s2] < c[v][s1] || leap[v][s1])) {
					found = 2;
					leap_start = s_1;
				}
			}
	}
	fleap_start = bli[v][leap_start];
	fleap_end = bli[v][s1];
	// Do not check tritone if it is at the end of not-last window
	if (ls == fli_size[v] - 2 && ep2 != c_len) return 0;
	if (found) {
		/*
		// Check if tritone is highest leap if this is last window
		if (ep2 == c_len && (av_cnt == 1 || !cantus_high)) {
			if ((cc[v][leap_start] >= lclimax[v][leap_start]) || (cc[v][s1] >= lclimax[v][leap_start])) {
				// Consecutive
				if (found == 1) FLAGVL(32, s0, fli[v][ls + 1]);
				// Compound framed
				else if (found == 2) FLAGVL(373, fli[v][ls - 1], fli[v][ls + 1]); //-V547
			}
		}
		*/
		// Check if resolution is correct
		GetTritoneResolution(ta, t1, t2, tb, res1, res2);
		// Flag resolution for consecutive tritone
		if (found == 1) {
			if (res1*res2 == 0)
				FLAGVL(31, s0, fli[v][ls + 1]);
			else FLAGVL(2, s0, fli[v][ls + 1]);
		}
		// Flag resolution for tritone with intermediate note, framed
		else if (found == 2) { //-V547
			if (res1*res2 == 0) FLAGVL(372, fli[v][ls - 1], fli[v][ls + 1]);
			else FLAGVL(371, fli[v][ls - 1], fli[v][ls + 1]);
		}
	}
	return 0;
}

int CP2R::FailTritones() {
	CHECK_READY(DR_pc, DR_c, DR_fli);
	CHECK_READY(DR_leap, DR_lclimax);
	for (ls = 0; ls < fli_size[v] - 1; ++ls) {
		s0 = fli[v][ls];
		s = fli2[v][ls];
		s1 = fli2[v][ls + 1];
		if (ls > 0) s_1 = fli2[v][ls - 1];
		if (ls > 1) s_2 = fli2[v][ls - 2];
		if (ls < fli_size[v] - 2) s2 = fli2[v][ls + 2];
		// Warning: tritone F#C in minor is not detected (can add FailTritone to detect) because it is already prohibited by Unaltered near altered.
		// If you allow Unaltered near altered, you should implement FailTritone for F#C.
		if (mode == 9) {
			if (FailTritone(3, 5, 11, 0)) return 1;
			if (FailTritone(7, 8, 2, 3)) return 1;
		}
		else {
			if (FailTritone(4, 5, 11, 0)) return 1;
		}
	}
	return 0;
}

// Calculate global fill
int CP2R::FailGlobalFill() {
	CHECK_READY(DR_nmin, DR_c);
	// Clear nstat2
	for (int i = nmind[v]; i <= nmaxd[v]; ++i) nstat2[i] = 0;
	// Count nstat2
	for (int x = 0; x < ep2; ++x) ++nstat2[c[v][x]];
	// Check nstat2
	if (ep2 < c_len) return 0;
	int skips = 0;
	int skips2 = 0;
	for (int x = nmind[v] + 1; x < nmaxd[v]; ++x) if (!nstat2[x]) {
		if (!nstat2[x + 1]) {
			++skips2;
			++x;
		}
		else ++skips;
	}
	// Set flags
	if (skips2) FLAGV(69, 0);
	if (skips == 1) FLAGV(67, 0);
	else if (skips >= 2) FLAGV(68, 0);
	return 0;
}