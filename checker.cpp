/* ADNA
 * Checker application
 */

#include <iostream>
#include <cstring>
#include "adnashm.hpp"
#include <curses.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <signal.h>
using namespace std;

int main() {
	/*cout << "-------------------------------------------" << endl;
	cout << "- adna -- current process checker         -" << endl;
	cout << "-------------------------------------------" << endl;*/

	int jobsize = sizeof(_job);
	int fdj = shm_open(JOBKEY, O_RDWR, 0666);
	void* mapptrj = mmap(NULL, jobsize, PROT_READ, MAP_SHARED, fdj, 0);
	if (mapptrj==MAP_FAILED) {
		cerr << "Checker Error: Unable to find job shared memory. Did you initialize a new job?" << endl;
		exit(1);
	}
	_job* myJob = static_cast<_job*>(mapptrj);
	
	int statsize = sizeof(_stat)*myJob->numProcs;
	int fdt = shm_open(TABKEY, O_RDWR, 0666);
	void* mapptrt = mmap(NULL, statsize, PROT_READ, MAP_SHARED, fdt, 0);
	if (mapptrt==MAP_FAILED) {
		cerr << "Checker Error: Unable to find stat shared memory. Did you initialize a new job?" << endl;
		exit(1);
	}
	_stat* myStat = static_cast<_stat*>(mapptrt);
	
	int ch;
	initscr();
	raw();
	nonl();
	noecho();
	intrflush(stdscr, FALSE);	
	(void)keypad(stdscr, TRUE);
	
	wclear(stdscr);
	wborder(stdscr, 0, 0, 0, 0, 0, 0, 0, 0);
	wmove(stdscr, 1, 2);
	waddstr(stdscr, (char*)"adna -- current job checker");
	wmove(stdscr, 2, 1);
	whline(stdscr, ACS_HLINE, COLS-2);
	//wmove(stdscr, 3, 1);
	//wprintw(stdscr, (char*)"This window is %d lines and %d columns", LINES, COLS);
	wmove(stdscr, LINES-3, 1);
	whline(stdscr, ACS_HLINE, COLS-2);
	wmove(stdscr, LINES-2, 2);
	waddstr(stdscr, (char*)"Press 'q' to exit and continue job. Press 'k' to exit and kill job.");
	
	string jobstr = "Job name: "+string(myJob->jobname);
	string fq1str = "FASTQ Read 1: "+string(myJob->fq1n);
	string fq2str = "FASTQ Read 2: "+string(myJob->fq2n);
	stringstream mpistr;
	mpistr << "Number of Processes: " << myJob->numProcs;
	
	wmove(stdscr, 3, 2);
	waddstr(stdscr, (char*)jobstr.c_str());
	wmove(stdscr, 4, 2);
	waddstr(stdscr, (char*)fq1str.c_str());
	wmove(stdscr, 5, 2);
	waddstr(stdscr, (char*)fq2str.c_str());
	wmove(stdscr, 6, 2);
	waddstr(stdscr, (char*)mpistr.str().c_str());
	
	wrefresh(stdscr);
	
	WINDOW * statscr = newwin(LINES-10, COLS-4, 7, 2);
	timeout(1000);
	
	int clear = 0;
	int error = 0;
	
	while(1) {
		wclear(statscr);
		wmove(statscr, 0, 0);
		time_t curtime = chrono::system_clock::to_time_t(chrono::system_clock::now());
		string timestr = "Last checked: "+string(ctime(&curtime));
		waddstr(statscr, (char*)timestr.c_str());
		
		clear = myJob->numProcs;
		int line = 1;
		for (int i=0; i<myJob->numProcs; i++) {
			wmove(statscr, line, 0);
			line++;
			stringstream procstat;
			procstat << "Proc " << i << " (" << myStat[i].PID << "): ";
			if (myStat[i].section == 0) {
				procstat << "Initialized and ready to start";
			} else if (myStat[i].section == 1) {
				procstat << "Crawling read ONE. Assigned " << myStat[i].readsAssigned << " reads";
			} else if (myStat[i].section == 2) {
				double progressdbl = (myStat[i].readsComplete / (double)myStat[i].readsAssigned) * 100;
				procstat << setprecision(2) << "Comparing with read TWO: " << myStat[i].readsComplete << " / " << myStat[i].readsAssigned << " (" << progressdbl << "%)";
			} else if (myStat[i].section == 3) {
				procstat << "Compiling metrics";
			} else if (myStat[i].section == 99) {
				clear--;
				procstat << "Completed " << myStat[i].readsComplete << " reads (assigned " << myStat[i].readsAssigned << ")";
			} else if (myStat[i].section < 0) {
				procstat << "ERROR: ";
				if (myStat[i].section == -1) procstat<<"Unable to set job shared memory";
				else if (myStat[i].section == -2) procstat<<"Unable to set stat shared memory";
				else if (myStat[i].section == -3) procstat<<"Process number mismatch. See log.";
				else if (myStat[i].section == -10) procstat<<"Cannot open file " << myJob->fq1n;
				else if (myStat[i].section == -11) procstat<<"R1 Bad Header. See log.";
				else if (myStat[i].section == -20) procstat<<"Cannot open file " << myJob->fq2n;
				else if (myStat[i].section == -21) procstat<<"R2 Bad Header. See log.";
				else if (myStat[i].section == -22) procstat<<"R2 Header did not find R1 match. See log.";
				else procstat << "Code " << myStat[i].section;
				clear--;
				error++;
			} else {
				procstat << "ERROR Unknown state: " << myStat[i].section;
			}
			waddstr(statscr, (char*)procstat.str().c_str());
		}
		
		wrefresh(statscr);
		
		if (ch == 'k' || ch == 'K' || ch == 'q' || ch == 'Q') break;
		if (clear == 0) { //all procs have status 99
			wmove(stdscr, LINES-2, 2);
			wclrtoeol(stdscr);
			if (error<1) waddstr(stdscr, (char*)"adna has completed the job! Press any key to exit.");
			else waddstr(stdscr, (char*)"ERROR! adna has encountered a problem. Press any key to exit.");
			timeout(-1);
			getch();
			break;
		}
		ch = getch();
	}
	
	endwin();
	
	if (clear==0) {
		stringstream cmd;
		const auto now = chrono::system_clock::now();
		const auto epoch = now.time_since_epoch();
		const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
		cmd << "mv ./results/curjob ./results/" << myJob->jobname << "-" << seconds.count();
		//cerr << "Executing: " << cmd.str() << endl;
		system(cmd.str().c_str());
		if (shm_unlink(JOBKEY)!=0) cerr << "Cleaner error: Unable to unlink job shared memory with: " << JOBKEY << endl;
		if (shm_unlink(TABKEY)!=0) cerr << "Cleaner error: Unable to unlink stat shared memory with: " << TABKEY << endl;
		if (error<1) cerr << "Exited adna-check with job completion.";
		else cerr << "Exited adna-check with job ERRORS.";
		cerr << " adna is NOT running and job info has been cleaned." << endl;
	}
	if (ch == 'q' || ch == 'Q') {
		cerr << "Exited adna-check without killing the adna job. adna is still running." << endl;
		
	} else if (ch == 'k' || ch == 'K') {
		for (int i=0; i<myJob->numProcs; i++) {
			if (myStat[i].PID > 0) {
				kill(myStat[i].PID, SIGTERM);
			}
		}
		if (shm_unlink(JOBKEY)!=0) cerr << "Cleaner error: Unable to unlink job shared memory with: " << JOBKEY << endl;
		if (shm_unlink(TABKEY)!=0) cerr << "Cleaner error: Unable to unlink stat shared memory with: " << TABKEY << endl;
		cerr << "Exited adna-check AND killed the adna job. adna is NOT running and job info has been cleaned." << endl;
	}
	
	return 0;
}







