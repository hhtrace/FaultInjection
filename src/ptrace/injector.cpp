/*
*  Author: HIT CS HDMC team.
*  Last modified by zhaozhilong: 2010-1-14
*/
#include "injector.h"
#include <sys/wait.h>
#include <signal.h>


int signalPid;	//for sigAlrm
int childProcess;
char * Injector::nameSignal( int signo )
{
	if( signo >= SIGRTMIN && signo <= SIGRTMAX )
	{
		return "reliable signal";
	}
	switch( signo )
	{
		case 1:
			return "SIGHUP";
		case 2:
			return "SIGINT";
		case 3:
			return "SIGQUIT";
		case 4:
			return "SIGILL";
		case 5:
			return "SIGTRAP";
		case 6:
			return "SIGABRT";	//or "SIGIOT"
		case 7:
			return "SIGBUS";
		case 8:
			return "SIGFPE";
		case 9:
			return "SIGKILL";
		case 10:
			return "SIGUSR1";
		case 11:
			return "SIGSEGV";
		case 12:
			return "SIGUSR2";
		case 13:
			return "SIGPIPE";
		case 14:
			return "SIGALRM";
		case 15:
			return "SIGTERM";
		case 16:
			return "SIGSTKFLT";
		case 17:
			return "SIGCHLD";
		case 18:
			return "SIGCONT";
		case 19:
			return "SIGSTOP";
		case 20:
			return "SIGTSTP";
		case 21:
			return "SIGTTIN";
		case 22:
			return "SIGTTOU";
		case 23:
			return "SIGURG";
		case 24:
			return "SIGXCPU";
		case 25:
			return "SIGXFSZ";
		case 26:
			return "SIGVTALRM";
		case 27:
			return "SIGPROF";
		case 28:
			return "SIGWINCH";
		case 29:
			return "SIGIO"; 	// or "SIGPOLL"
		case 30:
			return "SIGPWR";
		case 31:
			return "SIGSYS";
		default:
			cerr << "unknown signal" << endl;
			return "unknown";
	}
}

Injector::Injector()
{
	faultTablePath.clear();
	targetPid = -1;
	exeArguments = NULL;
}

Injector::~Injector()
{
	faultTable.clear();
}

Injector * Injector::initInjector( int argc, char **argv )
{
	Injector * pInjector = new Injector();
	if( pInjector == NULL ) { return NULL; }

	/*
     * union{
		int microsec;
		int step;
	};
	char unit;
	long faultPos;
	TYPE faultType;
    */
	//get arguments
	argc--;
	argv++;
    FAULT defaultFault(100000, 's', PT_F40, _2_bit_flip);
    //FAULT timeoutFault(3000000, 'm', NULL_POS, timeout);
    //3000000 microsecond null timeout

    while(argc > 0)
	{
#ifdef DEBUG
        cout <<argv[0] <<", " <<argv[1] <<endl;
#endif
        if(strcmp(argv[0], "--step") == 0)
        {
		    defaultFault.step = atoi(argv[1]);
        }
        else if(strcmp(argv[0], "--mode") == 0)
        {
		    if(strcmp(argv[1], "step") == 0 || strcmp(argv[1], "steps") == 0)
		    {
			    defaultFault.unit = 's';
		    }
		    else if(strcmp(argv[1], "microsecond") == 0 || strcmp(argv[1], "microseconds") == 0)
		    {
			    defaultFault.unit = 'm';
            }
        }
        else if(strcmp(argv[0], "--pos") == 0)
        {
		    defaultFault.faultPos = name2offset(argv[1]);
        }
        else if(strcmp(argv[0], "--type") == 0)
        {
		    defaultFault.faultType = fault2type(argv[1]);
        }
		else if(strcmp(argv[0],"-c") == 0)
		{
			pInjector->faultTablePath = argv[1];
		}
		else if(strcmp(argv[0],"-e") == 0)
		{
			pInjector->exeArguments = argv + 1;
			break;
		}
		else if(strcmp(argv[0],"-p") == 0)
		{
			pInjector->targetPid = atoi(argv[1]);
			break;
		}
		else
		{
			printf("Unknown option: %s\n",argv[0]);
			pInjector->usage();
			delete( pInjector );
			return NULL;
		}
		argc -= 2;
		argv += 2;
	}
    if(pInjector->faultTablePath == "")
    {
        pInjector->defaultFault = defaultFault;
        pInjector->faultTable.push_back(defaultFault);
        //pInjector->timeoutFault = timeoutFault;
        //pInjector->faultTable.push_back(timeoutFault);
    }
    else if( pInjector->initFaultTable( ) == RT_FAIL )
	{
		delete( pInjector );
		return NULL;
	}
	return pInjector;
}

int Injector::initFaultTable( )
{
	string line;
	if( faultTablePath.empty() )
	{
		cerr << "Error:no existing fault table" << endl;
		return RT_FAIL;
	}
	ifstream infile;
	infile.open( faultTablePath.c_str(), ios::in );
	if( !infile )
	{
		cerr << "Error:unable to open file:" << faultTablePath << endl;
		return RT_FAIL;
	}
	string strLine;
	string strTmp;
	FAULT  faultTmp;
	while( getline(infile,strLine,'\n') )
	{
		//bind istream to the strLine
		istringstream stream(strLine);

		// number of steps or microseconds
		strTmp.clear();
		stream >> strTmp;
		if( strTmp.empty() )
		{
			cerr << "Error:fault table format errno" << endl;
			return RT_FAIL;
		}
		faultTmp.step = atoi( strTmp.c_str() );
		// unit:step(s) or microsecond(s)
		strTmp.clear();
		stream >> strTmp;
		if( strTmp.empty() )
		{
				cerr << "Error:fault table format errno" << endl;
				return RT_FAIL;
		}
		else if( strTmp == "step" || strTmp == "steps" )
		{
			faultTmp.unit = 's';
		}
		else if( strTmp == "microsecond" || strTmp == "microseconds" )
		{
			faultTmp.unit = 'm';
		}
		else
		{
			cerr << "Error:undefined unit" << endl;
			return RT_FAIL;
		}
		// location:registers
		strTmp.clear();
		stream >> strTmp;
		if( strTmp.empty() )
		{
			cerr << "Error:fault table format errno" << endl;
			return RT_FAIL;
		}

		faultTmp.faultPos = name2offset(strTmp.c_str());
		if( faultTmp.faultPos == ERR_POS )
		{
			cerr << "Error:undefined fault location" << endl;
			return RT_FAIL;
		}

		// fault type:1_bit_flip,etc
		strTmp.clear();
		stream >> strTmp;
		if( strTmp.empty() )
		{
			cerr << "Error:fault table format errno" << endl;
			return RT_FAIL;
		}
		faultTmp.faultType = fault2type(strTmp);

		if(faultTmp.faultType == err_ftype)
		{
			cerr << "Error:undefined fault type" << endl;
			return RT_FAIL;
		}

		//add a fault into fault vector
		faultTable.push_back( faultTmp );
	}
	infile.close();
	return RT_OK;
}

void Injector::cleanup(void)
{
    //cout <<"childProcess " <<childProcess <<endl;
    if(childProcess == -1)
    {
        //cout <<"we should not kill the existing process..." <<endl;
        return;
    }
	int iRet = kill(childProcess, 0);
	if(iRet != -1)
	{
		iRet = kill(childProcess, SIGKILL);
		if(iRet < 0)
		{
			perror("kill");
		}
        //cout <<"kill child process [" <<childProcess <<"]" <<endl;
	}
}

void Injector::report(int signo)
{
	int iRet;
	cout <<"timeout now but the process is still running..." <<endl;

    writeResult(signalPid, TERM, 14);
    cleanup( );

    handleSigchld(SIGCHLD);
    //cout <<getpid( ) <<endl;
    exit(0);
    //cout <<getpid( ) <<endl;
}

void Injector::signaltimeout(int sec, void(*func)(int))
{
    //cout <<__func__ <<endl;
	signal(SIGALRM, func);
	alarm(sec);
}

int Injector::startInjection( void )
{
	int iRet;
	int data = 0;

    //signaltimeout(3000, sigAlrm);
	//inject fault into an existing process
	if( targetPid > 0 && exeArguments == NULL )
	{
		cout << "Inject into an exist process " << targetPid << endl;
        //设置跟踪进程，等待子进程停止
        signalPid = targetPid;		//用于给sigAlrm函数传递进程号
		childProcess = -1;		//用于给sigAlrm函数传递进程号 modify by gatieme by bug
		iRet = ptraceAttach( targetPid );
		if( iRet == RT_FAIL ) { return RT_FAIL; }

		do {
			iRet = procMonitor( targetPid,data );
			if( iRet == RT_FAIL ) { return RT_FAIL; }
		} while( iRet == RUN );

		//should be STOP
		if( iRet != STOP )
		{
			writeResult( targetPid, iRet, data );	//exit or term
			return RT_FAIL;
		}

		//进行故障注入
		iRet = injectFaults( targetPid, data );
		if( iRet != RT_OK ) { return RT_FAIL; }

		//继续执行
		ptraceCont( targetPid );

		//跟踪继续执行后的子进程
		while( 1 )
		{
			iRet = procMonitor( targetPid, data );
			if( iRet == RT_FAIL ) { return RT_FAIL; }

			if( iRet == RUN )
			{
				continue;
			}
			else if( iRet == STOP )
			{
				if( data == SIGTRAP ){ data = 0; }
				ptraceCont( targetPid, data );
			}
			else //exit or term
			{
				writeResult( targetPid, iRet, data );
				break;
			}
		}
		return RT_OK;
	}
	//inject fault into an excultable program
	if( exeArguments != NULL && targetPid < 0 )
	{
		cout << "Inject into an exculetable program " << exeArguments[0] << endl;
		errno = 0;

        /*if(signal(SIGCHLD, Injector::handleSigchld) == SIG_ERR)
        {
            perror("signal error : ");
            exit(-1);
        }*/
        pid_t child = fork( );
		if( child < 0 )
		{
			perror("fork");
			return RT_FAIL;
		}
		else if( child == 0 )	//child
		{
			signalPid = getpid( );
            childProcess = getpid( );
            //cout <<childProcess <<endl;
			iRet = ptraceTraceme();
			if( iRet == RT_FAIL )
			{
			    _exit(EXIT_ERR);
			}
			startExe();
			_exit( EXIT_OK );
		}
		else	//parent
		{
			signalPid = child;
            childProcess = child;
			do
            {
				iRet = procMonitor( child, data );
				if( iRet == RT_FAIL ) { return RT_FAIL; }
			} while( iRet == RUN );

			//should be STOP
			if( iRet != STOP )
			{
				writeResult( child, iRet, data );	//exit or term
				return RT_FAIL;
			}

			//进行故障注入
			iRet = injectFaults( child, data );
			if( iRet != RT_OK ) { return RT_FAIL; }

			//继续执行
			ptraceCont( child );

			//跟踪继续执行后的子进程
			while( 1 )
			{
				iRet = procMonitor( child, data );
				if( iRet == RT_FAIL ) { return RT_FAIL; }
				if( iRet == RUN ){ continue; }
				else if( iRet == STOP )
				{
					if( data == SIGTRAP ){ data = 0; }
					ptraceCont( child, data );
				}
				else //exit or term
				{
					//写结果文件
					writeResult( child, iRet, data );
					break;
				}
			}
			return RT_OK;
		}
	}

	cerr << "injection target is wrong" << endl;
	return RT_FAIL;
}

//RT_FAIL,进程已经结束
//RT_OK,进程没有结束
int Injector::injectFaults( int pid, int &status )
{
	unsigned int i = 0;
	int data = 0;
	int iRet;

	//fault information
	//cout << "----- Type : " << type2fault(faultTable[0].faultType) << " Pos : " << offset2name(faultTable[0].faultPos) << " -----" << endl;
    cout <<"There are " << faultTable.size( ) << " faults will be injected in faulttable..." << endl;
	for(i = 0 ; i<faultTable.size(); i++ )
	{
        faultTable[i].Show( );
		//tigger type
		if( faultTable[i].unit == 's' )	//run step
		{
			iRet = runStep( pid, faultTable[i].step );
			if( iRet != RT_OK ) { return RT_FAIL; }
		}
		else if( faultTable[i].unit == 'm' )	//run time
		{
			iRet = runTime( pid, faultTable[i].microsec );
			if( iRet != RT_OK ) { return RT_FAIL; }
		}
		else
		{
			return RT_FAIL;
		}

		if( faultTable[i].faultType == nop )
		{
			_nop();
		}
		else if( faultTable[i].faultType == timeout )
		{
	        /*if( pid > 0 && exeArguments == NULL ) // an existing process
            {
				writeResult( pid, iRet, data );
            }
            else
            {*/
			_timeout(pid);
			while( 1 )
			{
				iRet = procMonitor( pid, data );
				//cout << "iRet:" << iRet << ",data:" << data << endl;
				if( iRet == RT_FAIL ) { return RT_FAIL; }
				if( iRet == RUN )
                {
                    continue;
                }
				else if( iRet == STOP )
				{
					if( data == SIGTRAP ){ data = 0; }
					ptraceCont( pid, data );
				}
				else //exit or term
				{
					//写结果文件
					if(data == SIGKILL)
						data = SIGALRM;
					writeResult( pid, iRet, data );
					return RT_FAIL;
				}
			}
            //}
		}
		else
		{
			//run fault
			iRet = (*injectFuncs[faultTable[i].faultType])( pid, faultTable[i].faultPos );
			if( iRet == RT_FAIL ) { return RT_FAIL; }
		}
        signaltimeout(3, report);
	}
	return RT_OK;
}


//侦测进程状态
//返回值=RT_FAIL，函数失败
//返回值=RUN，进程没有停止或结束，正在执行或等待
//返回值=STOP，进程停止
//返回值=EXIT，进程正常退出,data为退出码
//返回值=TERM，进程异常终止，data为异常信号
int Injector::procMonitor( int pid, int &data )
{
	int iRet;
	int status;
	iRet = waitpid( pid, &status, WNOHANG | WCONTINUED );
	if( iRet == -1 )
	{
		perror( "waitpid" );
		return RT_FAIL;
	}
	else if( iRet == 0 )
	{
		return RUN;
	}
	else if(iRet == pid)
	{
		if( WIFSTOPPED( status ) )
		{
			data = WSTOPSIG( status );
			return STOP;
		}
		else if( WIFEXITED( status ) )
		{
			data = WEXITSTATUS( status );
			return EXIT;
		}
		else if( WIFSIGNALED( status ) )
		{
			data = WTERMSIG( status );
			return TERM;
		}
		else
		{
			return RT_FAIL;
		}
	}
	else
	{
		return RT_FAIL;
	}
}

void Injector::startExe()
{
	execv( exeArguments[0], exeArguments );
}

void Injector::usage()
{
	printf("\nArguments:\n");
	printf("\t1.fault description scripts,like 1_bit_pc.conf.\n");
	printf("\t2.workload,workload can be a executable program or a running process id.\n");
	printf("You should run the injector like this:\n");
	printf("\t./injector -c 1_bit_pc.conf -e program\n");
	printf("\t./injector -c 1_bit_pc.conf -p pid\n");
}

//RT_FAIL,进程已经结束
//RT_OK,进程没有结束
int Injector::runStep(int pid,int steps)
{
	int i;
	int iRet;
	int lastsig = 0;
	int data;

	for( i=0; i < steps ; i++ )
	{
		ptraceStep( pid, lastsig );
		iRet = waitpid(pid,&data,WUNTRACED | WCONTINUED);
		if(iRet < 0)
		{
			perror("waitpid");
			_exit(127);
		}
		if( WIFSTOPPED(data) )
		{
			lastsig = WSTOPSIG( data );
			if(lastsig == SIGTRAP)
			{
				lastsig = 0;
			}
			continue;
		}
		if( WIFEXITED(data) )
		{
			cout << i + 1 << " steps" << endl;
			writeResult( pid, EXIT, WEXITSTATUS(data) );
			return RT_FAIL;
		}
		if( WIFSIGNALED( data ) )
		{
			cout << i + 1 << " steps" << endl;
			writeResult( pid, TERM, WTERMSIG(data) );
			return RT_FAIL;
		}
	}
	cout << i << " steps" << endl;
	return RT_OK;
}

//RT_FAIL,进程已经结束
//RT_OK,进程没有结束
int Injector::runTime(int pid, int msecs)
{
	int ret;
	int status;
	int lastsig = 0;
	int startSec = 0;
	int startUsec = 0;
	int stopSec = 0;
	int stopUsec = 0;
	int timeSec = 0;
	int timeUsec = 0;
	int sec = msecs / 1000000;
	int usec = msecs % 1000000;
	if( 0 < sec || 0 < usec )
	{
		//捕捉定时器发出的SIGALRM信号
        struct sigaction sa;
        sa.sa_handler = Injector::sigAlrm;
		sa.sa_flags = 0;
		sigemptyset( &sa.sa_mask );
		sigaction( SIGALRM, &sa, (struct sigaction *)NULL );

		//cout << sec << "\t" << usec << endl;
		getTime( &startSec, &startUsec );
		//恢复执行
		ptraceCont( pid, lastsig );

		//打开定时器
		setTimer(sec,usec);
		//wait for signal
		do {
			ret = waitpid( pid, &status, WUNTRACED | WCONTINUED );
		}while ( (ret == -1 && errno == EINTR ) );

		setTimer(0,0);
		getTime(&stopSec, &stopUsec);

		timeSec = stopSec - startSec;
		timeUsec = stopUsec - startUsec;
		if( timeSec > 0 )
		{
			timeUsec = timeUsec + timeSec * 1000000;
		}

		cout << timeUsec << " microseconds" << endl;
        if(timeUsec > 3000000)
        {
            cout <<"it's timeout" <<endl;
        }

		if( WIFEXITED( status ) )
		{
			writeResult( pid, EXIT, WEXITSTATUS( status ) );
			return RT_FAIL;
		}
		if( WIFSIGNALED( status ) )
		{
			writeResult( pid, TERM, WTERMSIG( status ) );
			return RT_FAIL;
		}
		if ( WIFSTOPPED( status ) )
		{
			lastsig = WSTOPSIG( status );
			if( lastsig == SIGTRAP )
			{
				lastsig = 0;
			}
			else
			{
				writeResult( pid, TERM, WSTOPSIG( status ) );
				ptraceCont(pid,WSTOPSIG( status ));
				return RT_FAIL;
			}
		}
	}
	return RT_OK;
}

void Injector::getTime( int *pSec, int *pUsec )
{
	struct timeval tVal;
	gettimeofday( &tVal, (struct timezone *)NULL );
	*pSec = tVal.tv_sec;
	*pUsec = tVal.tv_usec;
}

void Injector::setTimer( int sec, int usec )
{
	struct itimerval itVal;

	/* interval */
	itVal.it_interval.tv_sec = 0;
	itVal.it_interval.tv_usec = 0;

	/* initial time */
	itVal.it_value.tv_sec = sec;
	itVal.it_value.tv_usec = usec;

	/* ITIMER_REAL type timer using real system time,send SIGALRM signal */
	setitimer( ITIMER_REAL, &itVal, (struct itimerval *)NULL );
}

void Injector::sigAlrm( int sig )
{
	/*if( targetPid > 0 && exeArguments == NULL )
    {
        cout << "an exist process should not be killed...";
        return ;
    }*/
	errno = 0;
	if( signalPid <= 0 )
	{
		cout << "Error:Illegal pid" << endl;
		return;
	}
	kill( signalPid, SIGTRAP );
	if( errno != 0 )
	{
		perror( "kill" );
	}
}

void Injector::writeResult( int pid, int status, int data )
{
	time_t localTime = time( NULL );
	tm lt = *localtime( &localTime );
	stringstream timeStamp;
	timeStamp << 1900 + lt.tm_year << "-" << 1 + lt.tm_mon << "-"	<< lt.tm_mday << " ";
	timeStamp << lt.tm_hour << ":" <<	lt.tm_min << ":" << lt.tm_sec;		//2009-10-23 16:23:12

	if( status == EXIT )
	{
		cout   << '[' << setw(19) << timeStamp.str() << ']' << "Process " << pid << " exited with code " << data << endl;
	}
	else if( status == TERM )
	{
		cout 	 << '[' << setw(19) << timeStamp.str() << ']' << "Process " << pid << " termed with signal " << data << "(" << nameSignal( data ) << ")"<< endl;
	}
}


void Injector::handleSigchld(int signo)
{
    pid_t   pid;
    int     state = -1;

    //cout <<__LINE__ <<"waitpid..." <<endl;
    while((pid = waitpid(-1, &state, WNOHANG)) < 0)
    {
        cout <<__LINE__ <<"waitpid..." <<endl;
    }
    //cout <<__LINE__ <<"waitpid..." <<endl;
    //cout <<"parent pid = " <<getpid( ) <<endl;
    //cout <<"parent ppid = " <<getppid( ) <<endl;;
    //cout <<"child process exited with status" << state <<endl;
    return;
}
