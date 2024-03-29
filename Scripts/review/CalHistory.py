#!/usr/bin/python

import sys
sys.path.append("..")
from ucnacore.LinFitter import *
from ucnacore.PyxUtils import *
from ucnacore.EncalDB import *
from ucnacore.QFile import *
from Asymmetries import runCal
import os
from datetime import datetime


class RuncalFile:
	def __init__(self,fname):
		rcals = [runCal(r) for r in QFile(fname).dat.get("runcal",[])]
		self.runcals = dict([(r.run,r) for r in rcals])
		self.runs = self.runcals.keys()
		self.runs.sort()

def plot_run_monitor(rlist,sname,tp="pedestal",outpath=None):

	rlist.sort()
	conn = open_connection()
	
	gdat = []
	for rn in rlist:
		if not rn%10:
			print rn
		cgwgid = getRunMonitorGIDs(conn,rn,sname,tp)
		if not cgwgid:
			print "*** Can't find run monitor for",rn,"***"
			continue	
		cgid,wgid = cgwgid
		cg = getGraph(conn,cgid)
		wg = getGraph(conn,wgid)
		if cg and wg:
			gdat.append([rn,cg[0][2],cg[0][3],wg[0][2],wg[0][3]])
	
	grmon=graph.graphxy(width=25,height=8,
				x=make_runaxis(rlist[0]-1,rlist[-1]+1),
				y=graph.axis.lin(title=sname),
				key = None)
	#grmon.texrunner.set(lfs='foils17pt')

	grmon.plot(graph.data.points(gdat,x=1,y=2,dy=4,title=None),
				[graph.style.errorbar(errorbarattrs=[rgb.blue,]),graph.style.symbol(symbol.circle,size=0.10,symbolattrs=[rgb.red,])])

	if outpath:
		grmon.writetofile(outpath+"/%s.pdf"%sname)
		
	return grmon
	
def plot_all_pedestals(rmin,rmax):

	rlist = getRunType(open_connection(),"Asymmetry",rmin,rmax)
	
	outpath = os.environ["UCNA_ANA_PLOTS"]+"/Pedestals/%i-%i/"%(rlist[0],rlist[-1])
	os.system("mkdir -p %s"%outpath)
	
	for s in ['E','W']:
		for t in range(4):
			sname = "ADC%s%iBeta"%(s,t+1)
			grmon = plot_run_monitor(rlist,sname,"pedestal",outpath)
		
		sname = "MWPC%sAnode"%s
		grmon = plot_run_monitor(rlist,sname,"pedestal",outpath)
				
		for p in ['x','y']:
			for c in range(16):
				sname = "MWPC%s%s%i"%(s,p,c+1)
				grmon = plot_run_monitor(rlist,sname,"pedestal",outpath)
	
			
def plot_trigeff_history(rmin,rmax):
	
	rlist = getRunType(open_connection(),"Asymmetry",rmin,rmax)
	outpath = os.environ["UCNA_ANA_PLOTS"]+"/Trigeff/%i-%i/"%(rlist[0],rlist[-1])
	os.system("mkdir -p %s"%outpath)
	conn = open_connection()
	
	for s in ['E','W']:
		for t in range(4):
			gdat = []
			for rn in rlist:
				tparms = list(getTrigeffParams(conn,rn,s,t))
				tparms.sort()
				try:
					gdat.append([rn,tparms[0][2],tparms[1][2],tparms[3][2],tparms[3][3]])
				except:
					print "***** Missing data for",rn,s,t
					continue
				if not rn%50:
					print rn
				
			
			gthresh=graph.graphxy(width=25,height=8,
						x=make_runaxis(rlist[0]-1,rlist[-1]+1),
						y=graph.axis.lin(title="Trigger ADC Threshold",min=0,max=100),
						key = None)
			gthresh.plot(graph.data.points(gdat,x=1,y=2,dy=3,title=None),
						[graph.style.errorbar(errorbarattrs=[rgb.blue,]),graph.style.symbol(symbol.circle,size=0.10,symbolattrs=[rgb.red,])])
			gthresh.writetofile(outpath+"/%s%i.pdf"%(s,t))
	
			geff=graph.graphxy(width=25,height=8,
						x=make_runaxis(rlist[0]-1,rlist[-1]+1),
						y=graph.axis.lin(title="Trigger Efficiency"),
						key = None)
			geff.plot(graph.data.points(gdat,x=1,y=4,dy=5,title=None),
						[graph.style.errorbar(errorbarattrs=[rgb.blue,]),graph.style.symbol(symbol.circle,size=0.10,symbolattrs=[rgb.red,])])
			geff.writetofile(outpath+"/Eff_%s%i.pdf"%(s,t))


def plot_ped_history():

	RF = RuncalFile(os.environ["UCNA_ANA_PLOTS"]+"/test/CalDump_2010_Asym.txt")
	RFs = RuncalFile(os.environ["UCNA_ANA_PLOTS"]+"/test/CalDump_2010_Sources.txt")
	
	runcals = [RF.runcals[r] for r in RF.runs]
	pdat = [(r, r.get_tubesparam("PedMean"), r.get_tubesparam("nPedPts"), r.get_tubesparam("PedRMS")) for r in runcals]
	runcalsS = [RFs.runcals[r] for r in RFs.runs]
	pdatS = [(r, r.get_tubesparam("PedMean"), r.get_tubesparam("nPedPts"), r.get_tubesparam("PedRMS")) for r in runcalsS]
			
	timeticks = (7*86400, 1*86400)
			
	myparter = graph.axis.parter.linear(tickdists=timeticks)
	mytexter = timetexter()
	mytexter.dateformat=r"$%m/%d$"
	gheight = 4
	goff = 0

	
	timeaxis = graph.axis.lin(title="date (2010)",parter=myparter,texter=mytexter,min=pdat[0][0].midTime()-86400,max=pdat[-1][0].midTime()+86400)
	c = canvas.canvas()
	glist = []
	
	for s in ['East','West']:
	
		for t in range(4):
			print s,t
			gdat = [(p[0].midTime(),p[1][(s[0],t)],p[2][(s[0],t)],p[3][(s[0],t)],p[0]) for p in pdat if p[2][(s[0],t)] > 10]
			gdatS = [(p[0].midTime(),p[1][(s[0],t)],p[2][(s[0],t)],p[3][(s[0],t)],p[0]) for p in pdatS]
			
			mu,sigma = musigma([g[1] for g in gdat])
			print gdat[0][0], gdat[-1][0], mu, sigma
			yrange = 5
			if sigma > 5:
				yrange = 50
				
			for g in gdat:
				if abs(g[1]-mu) > 5*sigma:
					print "Unusual pedestals ",g[-1].run,g[1],mu
			
			gaxis = timeaxis
			if glist:
				gaxis = graph.axis.linkedaxis(glist[0].axes["x"])
			grmon=graph.graphxy(width=25,height=gheight,ypos=goff,
				x=gaxis,
				y=graph.axis.lin(title="%s %i"%(s,t+1),min=mu-yrange,max=mu+yrange,parter=graph.axis.parter.linear(tickdists=[yrange,yrange/5])),
				key = None)
			setTexrunner(grmon)
			c.insert(grmon)
			glist.append(grmon)
			goff += gheight+0.1
			
			grmon.plot(graph.data.points(gdat,x=1,y=2,dy=4,title=None),
				[graph.style.errorbar(),graph.style.symbol(symbol.circle,size=0.10)])
			#grmon.plot(graph.data.points(gdatS,x=1,y=2,dy=4,title=None),
			#	[graph.style.errorbar(errorbarattrs=[rgb.red]),graph.style.symbol(symbol.circle,size=0.10,symbolattrs=[rgb.red])])
		
		goff += 0.5
		
	c.writetofile("/Users/michael/Desktop/PMTPedestals.pdf")

def plot_gms_history():

	RF = RuncalFile(os.environ["UCNA_ANA_PLOTS"]+"/test/CalDump_2010_Asym.txt")
	RFs = RuncalFile(os.environ["UCNA_ANA_PLOTS"]+"/test/CalDump_2010_Sources.txt")
	
	runcals = [RF.runcals[r] for r in RF.runs]
	pdat = [(r, r.get_tubesparam("Pulser0"), r.get_tubesparam("BiMean"), r.get_tubesparam("BiRMS"), r.get_tubesparam("nBiPts")) for r in runcals]
			
	timeticks = (7*86400, 1*86400)
			
	myparter = graph.axis.parter.linear(tickdists=timeticks)
	mytexter = timetexter()
	mytexter.dateformat=r"$%m/%d$"
	gheight = 4
	goff = 0

	
	timeaxis = graph.axis.lin(title="date (2010)",parter=myparter,texter=mytexter,min=pdat[0][0].midTime()-86400,max=pdat[-1][0].midTime()+86400)
	c = canvas.canvas()
	glist = []
	
	for s in ['East','West']:
	
		for t in range(4):
			print s,t
			k = (s[0],t)
			gdat = [(p[0].midTime(), 100*(p[2][k]/p[1][k]-1), 100*p[3][k]/p[1][k], p[0]) for p in pdat if p[4]>4]
			
			mu,sigma = musigma([g[1] for g in gdat])
			print gdat[0][0], gdat[-1][0], mu, sigma
			yrange = 12
							
			for g in gdat:
				if abs(g[1]-mu) > 5*sigma:
					print "Unusual pedestals ",g[-1].run,g[1],mu
			
			gaxis = timeaxis
			if glist:
				gaxis = graph.axis.linkedaxis(glist[0].axes["x"])
			grmon=graph.graphxy(width=25,height=gheight,ypos=goff,
				x=gaxis,
				y=graph.axis.lin(title="%s %i"%(s,t+1),min=-yrange,max=yrange), #parter=graph.axis.parter.linear(tickdists=[yrange,yrange/5])),
				key = None)
			setTexrunner(grmon)
			c.insert(grmon)
			glist.append(grmon)
			goff += gheight+0.1
			
			grmon.plot(graph.data.points(gdat,x=1,y=2,dy=3,title=None), [graph.style.symbol(symbol.circle,size=0.10)])
			grmon.plot(graph.data.function("y(x)=0",title=None), [graph.style.line(lineattrs=[style.linestyle.dashed,]),])


		goff += 0.5
		
	c.writetofile("/Users/michael/Desktop/PMT_BiGMS.pdf")



if __name__ == "__main__":
	
	#plot_all_pedestals(14000,16300)
	#plot_all_pedestals(16500,19900)
	
	#plot_ped_history()
	plot_gms_history()

	#plot_trigeff_history(14000,16300)
	#plot_trigeff_history(16500,19900)