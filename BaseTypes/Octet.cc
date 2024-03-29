#include "Octet.hh"
#include "SMExcept.hh"
#include <climits>
#include <utility>

AFPState afpForOctet(OctetRole t) {
	if(t == 1 || t==2 || t == 10 || t == 12 || t == 16 || t == 17 || t == 19 || t == 21)
		return AFP_OFF;
	if(t == 4 || t==5 || t == 7 || t == 9 || t == 13 || t == 14 || t == 22 || t == 24)
		return AFP_ON;
	if(t == 3 || t == 11 || t == 18 || t == 20)
		return AFP_OFF2ON;
	if(t == 6 || t == 8 || t == 15 || t == 23)
		return AFP_ON2OFF;
	return AFP_OTHER;
}

GVState gvForOctet(OctetRole t) {
	if(t == 1 || t==4 || t == 9 || t == 12 || t == 13 || t == 16 || t == 21 || t == 24 || t == OCTR_BG)
		return GV_CLOSED;
	if(t == 2 || t==5 || t == 7 || t == 10 || t == 14 || t == 17 || t == 19 || t == 22 || t == OCTR_FG)
		return GV_OPEN;
	return GV_OTHER;
}

RunType runTypeForOctet(OctetRole t) {
	if(t == 3 || t == 6 || t == 8 || t == 11 || t == 15 || t == 18 || t == 20 || t == 23)
		return DEPOL_RUN;
	if(OCTR_A1 <= t && t <= OCTR_B12)
		return ASYMMETRY_RUN;
	return UNKNOWN_RUN;
}

std::string nameForOctet(OctetRole t) {
	if(OCTR_A1 <= t && t <= OCTR_B12) {
		std::string s;
		if(t<=OCTR_A12)
			s = "A";
		else
			s = "B";
		s += itos(((t-OCTR_A1)%12)+1);
		return s;
	}
	if(t==OCTR_BG) return "NBG";
	if(t==OCTR_FG) return "NFG";
	return "N";
}

Octet::Octet(const Stringmap& m): divlevel(DIV_OCTET) {
	for(OctetRole t = OCTR_A1; t <= OCTR_B12; ++t) {
		std::vector<std::string> rlisting = m.retrieve(nameForOctet(t));
		for(std::vector<std::string>::iterator it = rlisting.begin(); it != rlisting.end(); it++) {
			std::vector<int> runlist = sToInts(*it);
			for(std::vector<int>::iterator rit = runlist.begin(); rit != runlist.end(); rit++)
				addRun(*rit,t);
		}
	}
}

void Octet::addRun(RunNum rn, OctetRole t) {
	std::map< OctetRole,std::vector<RunNum> >::iterator it = runs.find(t);
	if(it != runs.end()) {
		it->second.push_back(rn);
	} else {
		std::vector<RunNum> rns;
		rns.push_back(rn);
		runs.insert(std::make_pair(t,rns));
	}
}

std::vector<RunNum> Octet::getRuns(OctetRole t) const {
	std::map< OctetRole,std::vector<RunNum> >::const_iterator it = runs.find(t);
	if(it != runs.end())
		return it->second;
	return std::vector<RunNum>();
}

std::vector<RunNum> Octet::getAllRuns() const {
	std::vector<RunNum> rns;
	for(std::map< OctetRole,std::vector<RunNum> >::const_iterator it = runs.begin(); it != runs.end(); it++)
		for(std::vector<RunNum>::const_iterator rit = it->second.begin(); rit != it->second.end(); rit++)
			rns.push_back(*rit);
	std::sort(rns.begin(),rns.end());
	return rns;
}

std::vector<RunNum> Octet::getAsymRuns(bool foreground) const {
	std::vector<RunNum> rns;
	for(std::map< OctetRole,std::vector<RunNum> >::const_iterator it = runs.begin(); it != runs.end(); it++)
		if(runTypeForOctet(it->first)==ASYMMETRY_RUN && ((foreground && gvForOctet(it->first) == GV_OPEN) || (!foreground && gvForOctet(it->first) == GV_CLOSED)) )
			for(std::vector<RunNum>::const_iterator rit = it->second.begin(); rit != it->second.end(); rit++)
				rns.push_back(*rit);
	return rns;
}

Stringmap Octet::toStringmap() const {
	Stringmap s;
	for(std::map< OctetRole,std::vector<RunNum> >::const_iterator it = runs.begin(); it != runs.end(); it++) {
		std::vector<int> rlist;
		for(std::vector<RunNum>::const_iterator rit = it->second.begin(); rit != it->second.end(); rit++)
			rlist.push_back(*rit);
		s.insert(nameForOctet(it->first),vtos(rlist));
	}
	s.insert("name",octName());
	s.insert("afp",afpWords(octAFPState()));
	s.insert("divlevel",itos(divlevel));
	return s;
}

std::vector<Octet> Octet::getSubdivs(Octet_Division_t divlvl, bool onlyComplete) const {
	smassert(divlvl <= DIV_RUN);
	std::vector<Octet> pps;
	
	if(divlvl == DIV_RUN) {
		for(std::map< OctetRole,std::vector<RunNum> >::const_iterator it = runs.begin(); it != runs.end(); ++it) {
			for(std::vector<RunNum>::const_iterator rit = it->second.begin(); rit != it->second.end(); ++rit) {
				Octet pp;
				pp.addRun(*rit,it->first);
				pp.divlevel = divlvl;
				pps.push_back(pp);
			}
		}
	} else {
		unsigned int ndivs = 1 << divlvl;
		for(unsigned int n=0; n<ndivs; n++) {
			Octet pp;
			pp.divlevel = divlvl;
			bool completed = true;
			for(OctetRole t = OctetRole((24/ndivs)*n+1); t < OctetRole((24/ndivs)*(n+1)+1); ++t) {
				std::vector<RunNum> truns = getRuns(t);
				if(runTypeForOctet(t) == ASYMMETRY_RUN && !truns.size()) {
					completed = false;
					continue;
				}
				for(std::vector<RunNum>::iterator it = truns.begin(); it != truns.end(); it++)
					pp.addRun(*it,t);
			}
			if(!pp.getNRuns() || (onlyComplete && !completed))
				continue;
			pps.push_back(pp);
		}
	}
	
	return pps;
}

RunNum Octet::getFirstRun() const {
	RunNum rmin = INT_MAX;
	for(std::map< OctetRole,std::vector<RunNum> >::const_iterator it = runs.begin(); it != runs.end(); it++)
		for(std::vector<RunNum>::const_iterator rit = it->second.begin(); rit != it->second.end(); rit++)
			if(*rit<rmin)
				rmin=*rit;
	if(rmin==INT_MAX) return 0;
	return rmin;	
}

// comparison sorting operator for Octets, based on earliest run
bool operator<(const Octet& a, const Octet& b) {
	return a.getFirstRun() < b.getFirstRun();
}

std::vector<Octet> Octet::loadOctets(const QFile& q) {
	std::vector<Octet> os;
	std::vector<Stringmap> osin = q.retrieve("Octet");
	for(std::vector<Stringmap>::iterator it = osin.begin(); it != osin.end(); it++)
		os.push_back(Octet(*it));
	std::sort(os.begin(),os.end());
	return os;
}

Octet Octet::loadOctet(const QFile& q, unsigned int n) {
	std::vector<Octet> octs = loadOctets(q);
	if(n>=octs.size()) return Octet();
	return octs[n];
}

AFPState Octet::octAFPState() const {
	AFPState s = AFP_OTHER;
	for(std::map< OctetRole,std::vector<RunNum> >::const_iterator it = runs.begin(); it != runs.end(); it++) {
		AFPState s2 = afpForOctet(it->first);
		if(s2 == AFP_ON || s2 == AFP_OFF) {
			if( s != s2 && (s == AFP_ON || s == AFP_OFF))
				return AFP_OTHER;
			s = s2;
		}
	}
	return s;
}

std::string Octet::octName() const {
	if(!runs.size())
		return "NullOct";
	std::vector<RunNum> rns = getAllRuns();
	return itos(rns[0])+"-"+itos(rns.back())+"_"+itos(divlevel);
}
