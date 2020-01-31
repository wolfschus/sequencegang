/* Sequencegang V 4.0.53 DEV 30.01.2020
 * main.cpp
 *
 *  Created on: 29.04.2019
 *      Author: wolf
 * 
 * GNU General Public License v3.0
 * 
 */

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_rotozoom.h>
#include <SDL/SDL_mixer.h>
#include <string>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>
#include "rtmidi/RtMidi.h"
#include <chrono>
#include <fluidsynth.h>
#include <sqlite3.h>

#include "images/go-up.xpm"
#include "images/go-down.xpm"
#include "images/updown.xpm"
#include "images/go-next.xpm"
#include "images/go-previous.xpm"
#include "images/leftright.xpm"
#include "images/media-playback-start.xpm"
#include "images/media-playback-pause.xpm"
#include "images/media-playback-stop.xpm"
#include "images/document-save.xpm"
#include "images/gtk-edit.xpm"
#include "images/zoom-in.xpm"
#include "images/zoom-out.xpm"
#include "images/stop.xpm"
#include "images/list-add.xpm"
#include "images/list-remove.xpm"
#include "images/list-expand.xpm"
#include "images/folder.xpm"
#include "images/system-shutdown.xpm"
#include "images/edit-copy.xpm"
#include "images/edit-cut.xpm"
#include "images/edit-paste.xpm"
#include "images/edit-delete.xpm"
#include "images/go-bottom.xpm"
#include "images/go-top.xpm"
#include "images/repeat.xpm"
#include "images/plug.xpm"
#include "images/media-skip-backward.xpm"
#include "images/media-skip-forward.xpm"
#include "images/noteon.xpm"
#include "images/noteoff.xpm"
#include "images/speaker.xpm"
#include "images/muted.xpm"
#include "images/notmuted.xpm"
#include "images/opensample.xpm"

using namespace std;

int pattern[9][16][64][16][3];
struct devicesettings{
	char name[80];
	int mididevice;
	int midichannel;
	char type[5];   // drum; mono; poly
	int maxbank;
	int maxprog;
	char progtype[3];  // GM-GeneralMidi; SH-SH201; KG-Korg; XX-keine spezifische
};
devicesettings dstemp;
vector<devicesettings> dsettings;

struct drumsettings{
	char name[80];
	int note;
};

drumsettings drumtemp;
vector<drumsettings> drumsettings;

int seldev = 8;
int playdev = 0;
int anzahlplaydev = 1;
int playpat = 0;
bool playsong = false;
int selpat[8] = {0,0,0,0,0,0,0,0};
int oktave[18] = {3,3,3,3,3,3,-2,3,3,3,3,3,3,3,3,3,3,-2};
int volume[8] = {100,127,127,127,127,127,127,127};
int selprog[8] = {0,0,0,0,0,0,0,0};
int selbank[8] = {0,0,0,0,0,0,0,0};

int maxstep = 15;  // song[0][0][9]
int timedivision = 16;
int bpm = 120;
int songrepeat = 0;
bool newrepeat = false;
int aktstep = 64;
int oldstep = aktstep;
int songstep = 64;
int songtab = 0;
bool timerrun = false;
bool exttimerrun = false;
bool clockmodeext = false;
int midiclock = 0;
auto start = chrono::high_resolution_clock::now();
auto stop = chrono::high_resolution_clock::now();
vector<vector< unsigned char >> midiinmessages;
bool midioutdata = false;
bool midiindata = false;
bool midiinclockdata = false;
bool isnoteonoff[8] = {false,false,false,false,false,false,false,false};
int oldnote[8][8];
bool anzeige = true;
bool run = true;
bool ismuted[8] = {0,0,0,0,0,0,0,0};
char midiindevname[80] = "";
int midiindevice = 0;
int midiinch = 0;
char midiinclockdevname[80] = "";
int midiinclockdevice = 0;
int del_sample=255;
int sel_sample=255;
int maxdir=15;
int startdir=0;
int anzdir=0;
bool raspi = false;
bool debug = false;

struct cpuwerte{
	float idle;
	float usage;
};

cpuwerte oldcpu;
cpuwerte newcpu;
float cpuusage;
int anzahlcpu;
int cputimer = 0;
struct sysinfo memInfo;


struct soundbank{
	int key;
	char name[256];
	char path[256];
	Mix_Chunk* sound;
	int channel;
};

soundbank sndbnk[128];

vector<Mix_Chunk*> sounds;

fluid_synth_t* fluid_synth;
RtMidiOut *midiout = new RtMidiOut();

class WSMidi
{
public:
	void NoteOn(uint mididevice, int midichannel, int note, int volume)
	{
		vector<unsigned char> message;

		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(144+midichannel);
			message.push_back(note);
			message.push_back(volume);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
		return;
	}

	void NoteOff(uint mididevice, int midichannel, int note)
	{
		vector<unsigned char> message;
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(128+midichannel);
			message.push_back(note);
			message.push_back(0);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
		return;
	}

	void ProgramChange(uint mididevice, int midichannel, int program)
	{
		vector<unsigned char> message;
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(192+midichannel);
			message.push_back(program);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
		return;
	}

	void AllSoundsOff(uint mididevice, int midichannel)
	{
		vector<unsigned char> message;
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(176+midichannel);
			message.push_back(120);
			message.push_back(0);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
	}

	void AllNotesOff(uint mididevice, int midichannel)
	{
		vector<unsigned char> message;
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(176+midichannel);
			message.push_back(123);
			message.push_back(0);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
	}

	bool PlaySample(int note)
	{
		sndbnk[note].channel = Mix_PlayChannel(-1, sndbnk[note].sound, 0);
		Mix_Volume(sndbnk[note].channel,127);

		return true;
	}

	bool StopSample(int note)
	{
		Mix_HaltChannel(sndbnk[note].channel);
		sndbnk[note].channel=-1;

		return true;
	}

	void PlaySong()
	{
		if(playsong==false)
		{
			anzahlplaydev = 1;
		}
		else
		{
			anzahlplaydev = 8;
		}
		for(int i=0;i<anzahlplaydev;i++)
		{
			if(playsong==false)
			{
				playdev=seldev;
				playpat=selpat[seldev];
			}
			else
			{
				playdev=i;
				if(pattern[8][songtab][songstep][8][0]==4)
				{
					if(newrepeat==true)
					{
						songrepeat=pattern[8][songtab][songstep][8][1];
						newrepeat=false;
					}
				}
				if(pattern[8][songtab][songstep][8][0]==3)
				{
					timerrun=false;
					aktstep=64;
					songstep=64;
					for(int i=0;i<8;i++)
					{
						if(dsettings[i].mididevice==255)
						{
							fluid_synth_all_notes_off(fluid_synth, dsettings[i].midichannel-1);
						}
						else if(dsettings[i].mididevice==254)
						{
							// Sampler
						}
						else
						{
							AllNotesOff(dsettings[i].mididevice, dsettings[i].midichannel-1);
						}
					}
				}
				if(pattern[8][songtab][songstep][8][0]==2)
				{
					bpm=pattern[8][songtab][songstep][8][1]*4;
				}
				if(pattern[8][songtab][songstep][i][0]==1)
				{
					playpat=pattern[8][songtab][songstep][i][1];
				}
				else
				{
					playpat=255;
				}
			}
			if(ismuted[playdev]!=true)
			{
				if(playpat==16)
				{
					if(dsettings[playdev].mididevice==255)
					{
						fluid_synth_all_notes_off(fluid_synth, dsettings[playdev].midichannel-1);
					}
					else if(dsettings[playdev].mididevice==254)
					{
						// Sampler
					}
					else
					{
						AllNotesOff(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1);
					}
				}
				else if(playpat<16)
				{
					if(pattern[playdev][playpat][aktstep][15][0]==6)
					{
						if(dsettings[playdev].mididevice==255)
						{
							fluid_synth_program_change(fluid_synth, dsettings[playdev].midichannel-1,pattern[playdev][playpat][aktstep][15][1]);
						}
						else
						{
							ProgramChange(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1,pattern[playdev][playpat][aktstep][15][1]);
						}
					}
					if(strcmp(dsettings[playdev].type, "mono")==0)
					{
						// NoteOnOff NoteOff
						if((pattern[playdev][playpat][aktstep][0][0]==0 and isnoteonoff[0]==true) or pattern[playdev][playpat][aktstep][0][0]==2)
						{
							if(oldnote[playdev][0]!=255)
							{
								if(debug==true)
									cout << "NoteOff OnOff" << endl;
								if(dsettings[playdev].mididevice==255)
								{
									fluid_synth_noteoff(fluid_synth, dsettings[playdev].midichannel-1, oldnote[playdev][0]);
								}
								else if(dsettings[playdev].mididevice==254)
								{
									StopSample(oldnote[playdev][0]);
								}
								else
								{
									NoteOff(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, oldnote[playdev][0]);
								}
								oldnote[playdev][0] = 255;
								isnoteonoff[0]=false;
							}
						}
						// NoteOnOff NoteOn
						if(pattern[playdev][playpat][aktstep][0][0]==2)
						{
							if(debug==true)
								cout << "NoteOn OnOff" << endl;
							if(dsettings[playdev].mididevice==255)
							{
								fluid_synth_noteon(fluid_synth, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][0][1]),int(pattern[playdev][playpat][aktstep][0][2]));
							}
							else if(dsettings[playdev].mididevice==254)
							{
								PlaySample(int(pattern[playdev][playpat][aktstep][0][1]));
							}
							else
							{
								NoteOn(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][0][1]), int(pattern[playdev][playpat][aktstep][0][2]));
							}
							oldnote[playdev][0] = pattern[playdev][playpat][aktstep][0][1];
							isnoteonoff[0]=true;
						}
						// NoteOn
						if(pattern[playdev][playpat][aktstep][0][0]==7)
						{
							if(debug==true)
								cout << "NoteOn" << endl;
							if(dsettings[playdev].mididevice==255)
							{
								fluid_synth_noteon(fluid_synth, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][0][1]),int(pattern[playdev][playpat][aktstep][0][2]));
							}
							else if(dsettings[playdev].mididevice==254)
							{
								PlaySample(int(pattern[playdev][playpat][aktstep][0][1]));
							}
							else
							{
								NoteOn(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][0][1]), int(pattern[playdev][playpat][aktstep][0][2]));
							}
							oldnote[playdev][0] = pattern[playdev][playpat][aktstep][0][1];
						}
						// NoteOff
						if(pattern[playdev][playpat][aktstep][0][0]==8)
						{
							if(debug==true)
								cout << "NoteOff" << endl;
							if(oldnote[playdev][0]!=255)
							{
								if(dsettings[playdev].mididevice==255)
								{
									fluid_synth_noteoff(fluid_synth, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][0][1]));
								}
								else if(dsettings[playdev].mididevice==254)
								{
									StopSample(int(pattern[playdev][playpat][aktstep][0][1]));
								}
								else
								{
									NoteOff(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][0][1]));
								}
								oldnote[playdev][0] = 255;
							}
						}
					}
					if(strcmp(dsettings[playdev].type, "poly")==0)
					{
						for(int i=0;i<8;i++)
						{
							// NoteOnOff NoteOff
							if((pattern[playdev][playpat][aktstep][i][0]==0 and isnoteonoff[i]==true) or pattern[playdev][playpat][aktstep][i][0]==2)
							{
								if(oldnote[playdev][i]!=255)
								{
									if(dsettings[playdev].mididevice==255)
									{
										fluid_synth_noteoff(fluid_synth, dsettings[playdev].midichannel-1, oldnote[playdev][i]);
									}
									else if(dsettings[playdev].mididevice==254)
									{
										StopSample(oldnote[playdev][i]);
									}
									else
									{
										NoteOff(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, oldnote[playdev][i]);
									}
									oldnote[playdev][i] = 255;
									isnoteonoff[i]=false;
								}
							}
							// NoteOnOff NoteOn
							if(pattern[playdev][playpat][aktstep][i][0]==2)
							{
								if(dsettings[playdev].mididevice==255)
								{
									fluid_synth_noteon(fluid_synth, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][i][1]),int(pattern[playdev][playpat][aktstep][i][2]));
								}
								else if(dsettings[playdev].mididevice==254)
								{
									PlaySample(int(pattern[playdev][playpat][aktstep][i][1]));
								}
								else
								{
									NoteOn(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][i][1]), int(pattern[playdev][playpat][aktstep][i][2]));
								}
								oldnote[playdev][i] = pattern[playdev][playpat][aktstep][i][1];
								isnoteonoff[i]=true;
							}
							// NoteOn
							if(pattern[playdev][playpat][aktstep][i][0]==7)
							{
								if(dsettings[playdev].mididevice==255)
								{
									fluid_synth_noteon(fluid_synth, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][i][1]),int(pattern[playdev][playpat][aktstep][i][2]));
								}
								else if(dsettings[playdev].mididevice==254)
								{
									PlaySample(int(pattern[playdev][playpat][aktstep][i][1]));
								}
								else
								{
									NoteOn(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][i][1]), int(pattern[playdev][playpat][aktstep][i][2]));
								}
								oldnote[playdev][i] = pattern[playdev][playpat][aktstep][i][1];
							}
							// NoteOff
							if(pattern[playdev][playpat][aktstep][i][0]==8)
							{
								if(oldnote[playdev][i]!=255)
								{
									if(dsettings[playdev].mididevice==255)
									{
										fluid_synth_noteoff(fluid_synth, dsettings[playdev].midichannel-1, oldnote[playdev][i]);
									}
									else if(dsettings[playdev].mididevice==254)
									{
										StopSample(int(oldnote[playdev][i]));
									}
									else
									{
										NoteOff(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, oldnote[playdev][i]);
									}
									oldnote[playdev][i] = 255;
								}
							}
						}
					}
					if(strcmp(dsettings[playdev].type, "drum")==0)
					{
						for(int i=0;i<16;i++)
						{
							// NoteOn/NoteOff
							if(pattern[playdev][playpat][aktstep][i][0]==1)
							{
								if(dsettings[playdev].mididevice==255)
								{
									fluid_synth_noteon(fluid_synth, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][i][1]),int(pattern[playdev][playpat][aktstep][i][2]));
									fluid_synth_noteoff(fluid_synth, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][i][1]));
								}
								else
								{
									NoteOn(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][i][1]), int(pattern[playdev][playpat][aktstep][i][2]));
									NoteOff(dsettings[playdev].mididevice, dsettings[playdev].midichannel-1, int(pattern[playdev][playpat][aktstep][i][1]));
								}
							}
						}
					}
				}
			}
		}
		return;
	}
};

WSMidi wsmidi;

class ThreadClass
{
public:
   void run()
   {
      pthread_t thread;

      pthread_create(&thread, NULL, entry, this);
   }

   void UpdateMidiTimer()
   {
      while(1)
	  {
    	  usleep(60000000/timedivision*4/bpm);
    	  if(timerrun==true and clockmodeext==false)
    	  {
    		  aktstep++;
//    		  stop = chrono::high_resolution_clock::now();
//    		  std::chrono::duration<double> elapsed = stop - start;
//     		  cout << elapsed.count() << endl;
//    		  start = chrono::high_resolution_clock::now();
    		  if(aktstep>maxstep)
    		  {
    			  aktstep=0;
    			  if(songrepeat==0)
    			  {
    				  songstep++;
       				  newrepeat=true;
    			  }
    			  else
    			  {
    				  songrepeat--;
    			  }
    		  }
    		  if(songstep>63)
    		  {
    			  songstep=0;
    			  if(songtab<8)
    			  {
    				  songtab++;
    			  }
    			  if(songtab>7)
    			  {
    				  songtab=0;
    			  }
    		  }
    		  anzeige=true;
  			  wsmidi.PlaySong();
  			  oldstep=aktstep;
    	  }
	  }
   }

private:
   static void * entry(void *ptr)
   {
      ThreadClass *tc = reinterpret_cast<ThreadClass*>(ptr);
      tc->UpdateMidiTimer();
      return 0;
   }
};

static int soundbankcallback(void* data, int argc, char** argv, char** azColName)
{
//	cout << argv[0] << " : " << argv[1] << endl;
	char cstring[256];
	sprintf(sndbnk[stoi(argv[0])].name,"%s",argv[1]);
	sprintf(sndbnk[stoi(argv[0])].path,"%s",argv[2]);
	sprintf(cstring,"%s/%s",getenv("HOME"),argv[2]);
	sndbnk[stoi(argv[0])].sound=Mix_LoadWAV(cstring);

	return 0;
}

class ThreadCPUClass
{
public:
   void run()
   {
      pthread_t thread;

      pthread_create(&thread, NULL, entry, this);
   }

   void UpdateCPUTimer()
   {
      while(1)
	  {
    	  usleep(200000);
    	  oldcpu.idle=newcpu.idle;
    	  oldcpu.usage=newcpu.usage;
    	  newcpu=get_cpuusage();
    	  cpuusage=((newcpu.usage - oldcpu.usage)/(newcpu.idle + newcpu.usage - oldcpu.idle - oldcpu.usage))*100;
    	  sysinfo (&memInfo);
//    	  cout << memInfo.freeram/1024  << " - " <<  memInfo.bufferram/1024  << " - " <<  memInfo.sharedram/1024 << " - " << memInfo.totalram/1024 << endl;
//    	  cout << cpuusage << endl;
	  }
   }

   cpuwerte get_cpuusage()
   {
		ifstream fileStat("/proc/stat");
		string line;

		cpuwerte cpuusage;

		while(getline(fileStat,line))
		{
			if(line.find("cpu ")==0)
			{
				istringstream ss(line);
				string token;

				vector<string> result;
				while(std::getline(ss, token, ' '))
				{
					result.push_back(token);
				}
				cpuusage.idle=atof(result[5].c_str()) + atof(result[6].c_str());
				cpuusage.usage=atof(result[2].c_str()) + atof(result[3].c_str()) + atof(result[4].c_str()) + atof(result[7].c_str()) + atof(result[8].c_str()) + atof(result[9].c_str()) + atof(result[10].c_str()) + atof(result[11].c_str());
			}
		}
		return cpuusage;

   }

private:
   static void * entry(void *ptr)
   {
      ThreadCPUClass *tcc = reinterpret_cast<ThreadCPUClass*>(ptr);
      tcc->UpdateCPUTimer();
      return 0;
   }
};

static int settingscallback(void* data, int argc, char** argv, char** azColName)
{
	sprintf(dstemp.name,"%s",argv[1]);
	dstemp.mididevice=atoi(argv[2]);
	dstemp.midichannel=atoi(argv[3]);
	dstemp.maxbank=atoi(argv[4]);
	dstemp.maxprog=atoi(argv[5]);
	sprintf(dstemp.type,"%s",argv[6]);
	sprintf(dstemp.progtype,"%s",argv[7]);
	dsettings.push_back(dstemp);

	return 0;
}

static int writesettingscallback(void* data, int argc, char** argv, char** azColName)
{
	
	for(int i = 0; i<argc; i++) {
      cout << azColName[i] << ' : ' << argv[i] << endl;
   }

	return 0;
}

static int drumsettingscallback(void* data, int argc, char** argv, char** azColName)
{
	sprintf(drumtemp.name,"%s",argv[1]);
	drumtemp.note=atoi(argv[2]);
	drumsettings.push_back(drumtemp);

	return 0;
}

static int midiinsettingscallback(void* data, int argc, char** argv, char** azColName)
{
	// midiin
	if(atoi(argv[0])==1)
	{
		sprintf(midiindevname,"%s",argv[1]);
		midiindevice=atoi(argv[2]);
		midiinch=atoi(argv[3]);
	}
	// midiinclock
	else if(atoi(argv[0])==2)
	{
		sprintf(midiinclockdevname,"%s",argv[1]);
		midiinclockdevice=atoi(argv[2]);
	}

	return 0;
}

void midiincallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
	unsigned int nBytes = message->size();
	if((int)message->at(0)<248)
	{
		midiinmessages.push_back(*message);
	}
	midiindata=true;
}

void midiinclockcallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
	unsigned int nBytes = message->size();
	if(clockmodeext==true and nBytes > 0 )
	{
		if((int)message->at(0)==250)
		{
			exttimerrun = true;
		}
		if((int)message->at(0)==251)
		{
			exttimerrun = true;
		}
		if((int)message->at(0)==252)
		{
			exttimerrun = false;
		}
		if(exttimerrun==true)
		{
			midiclock++;
			if(midiclock>=128/timedivision)
			{
				midiclock=0;
				aktstep++;
				if(aktstep>maxstep)
				{
					aktstep=0;
					if(songrepeat==0)
					{
					  songstep++;
					  newrepeat=true;
					}
					else
					{
					  songrepeat--;
					}
				}
				if(songstep>63)
				{
					songstep=0;
					if(songtab<8)
					{
						songtab++;
					}
					if(songtab>7)
					{
						songtab=0;
					}
				}
			}
  			wsmidi.PlaySong();
  			oldstep=aktstep;
		}
	}
}

bool CheckMouse(int mousex, int mousey, SDL_Rect Position)
{
	if( ( mousex > Position.x ) && ( mousex < Position.x + Position.w ) && ( mousey > Position.y ) && ( mousey < Position.y + Position.h ) )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int main(int argc, char* argv[])
{
	bool debug=false;
	bool fullscreen=false;
	
	// Argumentverarbeitung
	for (int i = 0; i < argc; ++i)
	{
		if(string(argv[i])=="--help")
		{
			cout << "Arturiagang" << endl;
			cout << "(c) 2019 - 2020 by Wolfgang Schuster" << endl;
			cout << "arturiagang --fullscreen = fullscreen" << endl;
			cout << "arturiagang --debug = debug" << endl;
			cout << "arturiagang --help = this screen" << endl;
			SDL_Quit();
			exit(0);
		}
		if(string(argv[i])=="--fullscreen")
		{
			fullscreen=true;
		}
		if(string(argv[i])=="--debug")
		{
			debug=true;
		}
	}

	ThreadClass tc;
	tc.run();

	ThreadCPUClass tcc;
	tcc.run();

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1)
	{
		std::cerr << "Konnte SDL nicht initialisieren! Fehler: " << SDL_GetError() << std::endl;
		return -1;
	}
	SDL_Surface* screen;
	if(fullscreen==true)
	{
		screen = SDL_SetVideoMode(1024, 600 , 32, SDL_DOUBLEBUF|SDL_FULLSCREEN);
	}
	else
	{
		screen = SDL_SetVideoMode(1024, 600 , 32, SDL_DOUBLEBUF);
	}
	if(!screen)
	{
	    std::cerr << "Konnte SDL-Fenster nicht erzeugen! Fehler: " << SDL_GetError() << std::endl;
	    return -1;
	}
	int scorex = screen->w/40;
	int scorey = screen->h/24;
	if(TTF_Init() == -1)
	{
	    std::cerr << "Konnte SDL_ttf nicht initialisieren! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	TTF_Font* fontbold = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", int(0.6*scorey));
	if(!fontbold)
	{
	    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", int(0.6*scorey));
	if(!font)
	{
	    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	TTF_Font* fontsmall = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", int(0.5*scorey));
	if(!fontsmall)
	{
	    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	if(Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096) == -1)
	{
	    std::cerr << "Konnte SDL_mixer nicht starten! Fehler: " << Mix_GetError() << std::endl;
	    return -1;
	}

	SDL_WM_SetCaption("Sequencegang", "Sequencegang");

	bool keyPressed[SDLK_LAST];
	memset(keyPressed, 0, sizeof(keyPressed));
	SDL_EnableUNICODE(1);

	SDL_Color textColor = {0xFF, 0xFF, 0xFF};
	SDL_Color blackColor = {0x00, 0x00, 0x00};
	SDL_Surface* text = NULL;
	SDL_Rect textPosition;

	char temptext[80];

	time_t zeitstempel;
	tm *now;
	char aktzeit[80];
	char tmpzeit[80];
	int wstimer = 0;

	DIR *dir;
	struct dirent *ent;
	vector<string> directory;
	vector<string> songdir;
	vector<string> sampledir;

	RtMidiIn *midiin = new RtMidiIn();
	RtMidiIn *midiinclock = new RtMidiIn();
	vector<unsigned char> message;
	vector<unsigned char> cc;
	vector<unsigned char> inmessage;
	message.push_back(0);
	message.push_back(0);
	message.push_back(0);
	vector<string> midioutname;
	vector<string> midiinname;
	int klavlastnote = 255;
	int nBytes;
	double stamp;

	// Check available ports.
	int onPorts = midiout->getPortCount();
	if ( onPorts == 0 )
	{
	  cout << "No ports available!\n";
	}
	else
	{
		for(int i=0;i<onPorts;i++)
		{
			midioutname.push_back(midiout->getPortName(i));
		}
	}
	// Check available ports.
	int inPorts = midiin->getPortCount();
	if ( inPorts == 0 )
	{
	  cout << "No ports available!\n";
	}
	else
	{
		for(int i=0;i<inPorts;i++)
		{
			midiinname.push_back(midiin->getPortName(i));
		}
	}

	SDL_Surface* up_image_org = IMG_ReadXPMFromArray(go_up_xpm);
	SDL_Surface* up_image = zoomSurface(up_image_org,float(scorex)/48,float(scorey)/48,1);
	SDL_Rect up_button;
	SDL_Surface* down_image_org = IMG_ReadXPMFromArray(go_down_xpm);
	SDL_Surface* down_image = zoomSurface(down_image_org,float(scorex)/48,float(scorey)/48,1);
	SDL_Rect down_button;
	SDL_Surface* updown_image_org = IMG_ReadXPMFromArray(updown_xpm);
	SDL_Surface* updown_image = zoomSurface(updown_image_org,float(scorex)/48,float(scorey)/48,1);
	SDL_Rect updown_button;
	SDL_Rect updown_area;
	updown_area.x = 39*scorex;
	updown_area.y = 6*scorey;
	updown_area.w = scorex;
	updown_area.h = 10*scorey;
	bool updown_aktiv = false;

	SDL_Surface* right_image_org = IMG_ReadXPMFromArray(go_next_xpm);
	SDL_Surface* right_image = zoomSurface(right_image_org,float(scorex)/48,float(scorey)/48,1);
	SDL_Rect right_button;
	SDL_Surface* left_image_org = IMG_ReadXPMFromArray(go_previous_xpm);
	SDL_Surface* left_image = zoomSurface(left_image_org,float(scorex)/48,float(scorey)/48,1);
	SDL_Rect left_button;
	SDL_Surface* leftright_image_org = IMG_ReadXPMFromArray(leftright_xpm);
	SDL_Surface* leftright_image = zoomSurface(leftright_image_org,float(scorex)/48,float(scorey)/48,1);
	SDL_Rect leftright_button;
	SDL_Rect leftright_area;

	SDL_Surface* play_image_org = IMG_ReadXPMFromArray(media_playback_start_xpm);
	SDL_Surface* play_image = zoomSurface(play_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect play_button;
	SDL_Surface* pause_image_org = IMG_ReadXPMFromArray(media_playback_pause_xpm);
	SDL_Surface* pause_image = zoomSurface(pause_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Surface* stop_image_org = IMG_ReadXPMFromArray(media_playback_stop_xpm);
	SDL_Surface* stop_image = zoomSurface(stop_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect stop_button;
	SDL_Surface* plug_image_org = IMG_ReadXPMFromArray(plug_xpm);
	SDL_Surface* plug_image = zoomSurface(plug_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect plug_button;
	SDL_Surface* backward_image_org = IMG_ReadXPMFromArray(media_skip_backward_xpm);
	SDL_Surface* backward_image = zoomSurface(backward_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect backward_button;
	SDL_Surface* forward_image_org = IMG_ReadXPMFromArray(media_skip_forward_xpm);
	SDL_Surface* forward_image = zoomSurface(forward_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect forward_button;
	SDL_Surface* allnotesoff_image_org = IMG_ReadXPMFromArray(stop_xpm);
	SDL_Surface* allnotesoff_image = zoomSurface(allnotesoff_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect allnotesoff_button;

	SDL_Surface* save_image_org = IMG_ReadXPMFromArray(document_save_xpm);
	SDL_Surface* save_image = zoomSurface(save_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect save_button;
	SDL_Surface* edit_image_org = IMG_ReadXPMFromArray(gtk_edit_xpm);
	SDL_Surface* edit_image = zoomSurface(edit_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect edit_button;
	SDL_Surface* load_image_org = IMG_ReadXPMFromArray(folder_xpm);
	SDL_Surface* load_image = zoomSurface(load_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect load_button;
	SDL_Surface* shutdown_image_org = IMG_ReadXPMFromArray(system_shutdown_xpm);
	SDL_Surface* shutdown_image = zoomSurface(shutdown_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect shutdown_button;
	bool isedit = false;

	SDL_Surface* speaker_image_org = IMG_ReadXPMFromArray(speaker_xpm);
	SDL_Surface* speaker_image = zoomSurface(speaker_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect speaker_button;

	SDL_Surface* noteadd_image_org = IMG_ReadXPMFromArray(list_add_xpm);
	SDL_Surface* noteadd_image = zoomSurface(noteadd_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect noteadd_button;
	SDL_Surface* noteexpand_image_org = IMG_ReadXPMFromArray(list_expand_xpm);
	SDL_Surface* noteexpand_image = zoomSurface(noteexpand_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect noteexpand_button;
	SDL_Surface* notedel_image_org = IMG_ReadXPMFromArray(list_remove_xpm);
	SDL_Surface* notedel_image = zoomSurface(notedel_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect notedel_button;
	SDL_Surface* noteon_image_org = IMG_ReadXPMFromArray(noteon_xpm);
	SDL_Surface* noteon_image = zoomSurface(noteon_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect noteon_button;
	SDL_Surface* noteoff_image_org = IMG_ReadXPMFromArray(noteoff_xpm);
	SDL_Surface* noteoff_image = zoomSurface(noteoff_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect noteoff_button;

	SDL_Surface* oktavedown_image_org = IMG_ReadXPMFromArray(go_bottom_xpm);
	SDL_Surface* oktavedown_image = zoomSurface(oktavedown_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect oktavedown_button;
	SDL_Surface* oktaveup_image_org = IMG_ReadXPMFromArray(go_top_xpm);
	SDL_Surface* oktaveup_image = zoomSurface(oktaveup_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect oktaveup_button;
	SDL_Surface* editcopy_image_org = IMG_ReadXPMFromArray(edit_copy_xpm);
	SDL_Surface* editcopy_image = zoomSurface(editcopy_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect editcopy_button;
	SDL_Surface* editcut_image_org = IMG_ReadXPMFromArray(edit_cut_xpm);
	SDL_Surface* editcut_image = zoomSurface(editcut_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect editcut_button;
	SDL_Surface* editpaste_image_org = IMG_ReadXPMFromArray(edit_paste_xpm);
	SDL_Surface* editpaste_image = zoomSurface(editpaste_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect editpaste_button;
	bool ispaste = false;

	SDL_Surface* plus_image_org = IMG_ReadXPMFromArray(zoom_in_xpm);
	SDL_Surface* plus_image = zoomSurface(plus_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect plus_button;
	SDL_Surface* minus_image_org = IMG_ReadXPMFromArray(zoom_out_xpm);
	SDL_Surface* minus_image = zoomSurface(minus_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect minus_button;
	SDL_Surface* ende_image_org = IMG_ReadXPMFromArray(stop_xpm);
	SDL_Surface* ende_image = zoomSurface(ende_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect ende_button;
	SDL_Surface* repeat_image_org = IMG_ReadXPMFromArray(repeat_xpm);
	SDL_Surface* repeat_image = zoomSurface(repeat_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
	SDL_Rect repeat_button;

	SDL_Surface* muted_image_org = IMG_ReadXPMFromArray(muted_xpm);
	SDL_Surface* muted_image = zoomSurface(muted_image_org,float(2*scorex-2)/96,float(2*scorey-2)/96,1);
	SDL_Surface* notmuted_image_org = IMG_ReadXPMFromArray(notmuted_xpm);
	SDL_Surface* notmuted_image = zoomSurface(notmuted_image_org,float(2*scorex-2)/96,float(2*scorey-2)/96,1);
	SDL_Rect mute_button[8];

	SDL_Surface* sampledelete_image = IMG_ReadXPMFromArray(edit_delete_xpm);
	sampledelete_image = zoomSurface(sampledelete_image,float(2*scorex-2)/96,float(2*scorey-2)/96,1);
	SDL_Surface* opensample_image = IMG_ReadXPMFromArray(opensample_xpm);
	opensample_image = zoomSurface(opensample_image,float(2*scorex-2)/96,float(2*scorey-2)/96,1);
	SDL_Rect opensample_button[12];

	bool leftright_aktiv = false;

	anzahlcpu=get_nprocs();
	sysinfo (&memInfo);

	SDL_Rect klavier[12];
	int selkey = 255;

	int startanzeigepattern = 0;
	int songanzeigepattern = 0;

	SDL_Rect ok_button;
	SDL_Rect cancel_button;
	int selloadsong = 0;
	int selloadsample = 0;
	SDL_Rect load_window;
	bool blink = true;
	int blinktimer = 0;

	SDL_Rect midiindev_rect;
	SDL_Rect midiinch_rect;
	SDL_Rect midiinclockdev_rect;

	fstream file;

	vector<SDL_Rect> settings_devname_rect;
	vector<SDL_Rect> settings_mididevice_rect;
	vector<SDL_Rect> settings_midichannel_rect;
	vector<SDL_Rect> settings_miditype_rect;
	SDL_Rect settings_midiindevname_rect;

	char sql[512];
	char songpath[512];
	char samplepath[512];

	// Load SettingsDB
	sqlite3 *settingsdb;

	char dbpath[1024];
	sprintf(dbpath, "%s/.sequencegang/settings.db", getenv("HOME"));

	if(sqlite3_open(dbpath, &settingsdb) != SQLITE_OK)
	{
		cout << "Fehler beim Öffnen: " << sqlite3_errmsg(settingsdb) << endl;
		return 1;
	}
	cout << "Settingsdatenbank erfolgreich geöffnet!" << endl;

	//Settings
	sprintf(sql, "SELECT * FROM settings");
	if( sqlite3_exec(settingsdb,sql,settingscallback,0,0) != SQLITE_OK)
	{
		cout << "Fehler beim SELECT: " << sqlite3_errmsg(settingsdb) << endl;
		return 1;
	}

	//DrumSettings
	sprintf(sql, "SELECT * FROM drumsettings");
	if( sqlite3_exec(settingsdb,sql,drumsettingscallback,0,0) != SQLITE_OK)
	{
		cout << "Fehler beim SELECT: " << sqlite3_errmsg(settingsdb) << endl;
		return 1;
	}

	//MidiInSettings
	sprintf(sql, "SELECT * FROM midiinsettings");
	if( sqlite3_exec(settingsdb,sql,midiinsettingscallback,0,0) != SQLITE_OK)
	{
		cout << "Fehler beim SELECT: " << sqlite3_errmsg(settingsdb) << endl;
		return 1;
	}

	sqlite3_close(settingsdb);

	sqlite3 *soundbankdb;
	sprintf(dbpath, "%s/.sequencegang/samplebank.db", getenv("HOME"));
	if(sqlite3_open(dbpath, &soundbankdb) != SQLITE_OK)
	{
		cout << "Fehler beim Öffnen: " << sqlite3_errmsg(soundbankdb) << endl;
		return 1;
	}
	cout << "Sampledatenbank erfolgreich geöffnet!" << endl;
	
	for(int i=0;i<128;i++)
	{
		sndbnk[i].key=i;
		sprintf(sndbnk[i].name,"%s", "X");
		sndbnk[i].sound=NULL;
		sndbnk[i].channel=-1;
		sprintf(sql, "SELECT key,name,path FROM samples WHERE key = %d;",i);
		if( sqlite3_exec(soundbankdb,sql,soundbankcallback,0,0) != SQLITE_OK)
		{
			cout << "Fehler beim SELECT: " << sqlite3_errmsg(soundbankdb) << endl;
			return 1;
		}
	}

	sqlite3_close(soundbankdb);

	// Pattern [Device][Pattern][Step][BefehlNr][Befehl];
	// Befehl[0]: 0 - Null; 1 - NoteOn/NoteOff; 2 - NoteOn/Note Off Ende; 3 - Hold Note On; 4 -  Spare ;5 - Bank; 6 - Program; 7 - Note On; 8 - Note Off
	// Befehl[1]: 0 - 127 (NoteOn, NoteOff: Note; Bank: BankNr; Program : ProgramNr;)
	// Befehl[2]: 0 - 127 (NoteOn, NoteOff: Velocity)

	// Songpattern
	// 0-7
	// Befehl[0]: 0 - nichts; 1 - Pattern;
	// Befehl[1]: 0-15 TabNr.
	// 8
	// Befehl[0]: 2 - BPM; 3 - Song end;
	// Befehl[1]: 0-255 *2 BPM
	// 9
	// Befehl[0]: 4 - time division;
	// Befehl[1]: 4;8;16;32
	// 10
	// Befehl[0]: 5 - steps;
	// Befehl[1]: 1-64


	int temppattern[64][16][3];

//	pattern[1][0][0][15][0]=6;
//	pattern[1][0][0][15][1]=81;

	for(int i=0;i<9;i++)
		for(int j=0;j<16;j++)
			for(int k=0;k<64;k++)
				for(int l=0;l<16;l++)
					for(int m=0;m<3;m++)
					{
						pattern[i][j][k][l][m]=0;
						temppattern[k][l][m]=0;
					}

	int selpatt[4] = {255,255,255,255};
	SDL_Rect timedevrect;
	SDL_Rect maxsteprect;
	SDL_Rect bpmrect;

	SDL_Rect insertNoteOnOff_rect;
	SDL_Rect insertNoteOn_rect;
	SDL_Rect insertNoteHold_rect;
	SDL_Rect deleteNote_rect;

	for(int i=0;i<8;i++)
	{
		for(int j=0;j<8;j++)
		{
			oldnote[i][j]=255;
		}

	}

	SDL_Rect volumerect;
	SDL_Rect progrect;
	SDL_Rect bankrect;
	char note[12][3] = {"B", "A#", "A", "G#", "G", "F#", "F", "E", "D#", "D", "C#", "C"};
	char songname[80] = "New Song";
	char samplename[80] = "";
	char texteingabe[80] = "";
	int seldevname = 0;
	char textinputtitel[80] = "Text Input";
	bool textinputmidiout = false;
	bool textinputmidiin = false;

	int launchkey_tabmode = 0;

	int mousex = 0;
	int mousey = 0;
	int mousescorex = 0;
	int mousescorey = 0;

	auto leftclick = chrono::high_resolution_clock::now();
	auto lastleftclick = chrono::high_resolution_clock::now();
	bool isleftdclick = false;
	bool stillleftclick = false;

	if(midiindevice<inPorts)
	{
		midiin->openPort( midiindevice );
		midiin->setCallback( &midiincallback );
		// Don't ignore sysex, timing, or active sensing messages.
		midiin->ignoreTypes( false, false, false );
	}
	if(midiinclockdevice<inPorts)
	{
		midiinclock->openPort( midiinclockdevice );
		midiinclock->setCallback( &midiinclockcallback );
		// Don't ignore sysex, timing, or active sensing messages.
		midiinclock->ignoreTypes( false, false, false );
	}

	// Fluidsynth
    fluid_settings_t* fluid_settings = new_fluid_settings();
    fluid_audio_driver_t* adriver;

	struct fluid_program {
    	unsigned int channel;
		unsigned int bank;
		unsigned int program;
	};
	fluid_program fptmp;
	vector<fluid_program> fluid_program_state;

    int sf2id;
    char* fluid_alsa_device;
	unsigned int sfid = 0;
	unsigned int bank = 0;
	unsigned int program = 0;
	vector<SDL_Rect> fluid_settings_bank_rect;
	vector<SDL_Rect> fluid_settings_program_rect;

    fluid_settings_setint(fluid_settings, "synth.polyphony", 128);
    fluid_synth = new_fluid_synth(fluid_settings);
    fluid_settings_setstr(fluid_settings, "audio.driver", "alsa");
    adriver = new_fluid_audio_driver(fluid_settings, fluid_synth);
    sf2id = fluid_synth_sfload(fluid_synth,"/usr/share/sounds/sf2/FluidR3_GM.sf2",true);
    fluid_synth_program_change(fluid_synth, 0 , 0);

    int fluid_nmid_chan = fluid_synth_count_midi_channels(fluid_synth);
    fluid_settings_getstr(fluid_settings, "audio.alsa.device", &fluid_alsa_device);

    for(int i=0;i<16;i++)
    {
		if (FLUID_OK == fluid_synth_get_program (fluid_synth, i, &sfid, &bank, &program))
		{
			fptmp.channel=i;
			fptmp.bank=bank;
			fptmp.program=program;
			fluid_program_state.push_back(fptmp);
		}
    }

// GM Instruments
	vector<char *> gm_program_name;
	gm_program_name.push_back("Acoustic Piano");
	gm_program_name.push_back("Bright Piano");
	gm_program_name.push_back("Electric Grand Piano");
	gm_program_name.push_back("Honky-tonk Piano");
	gm_program_name.push_back("Electric Piano 1");
	gm_program_name.push_back("Electric Piano 2");
	gm_program_name.push_back("Harpsichord");
	gm_program_name.push_back("Clavi");
	gm_program_name.push_back("Celesta");
	gm_program_name.push_back("Glockenspiel");
	gm_program_name.push_back("Musical box");
	gm_program_name.push_back("Vibraphone");
	gm_program_name.push_back("Marimba");
	gm_program_name.push_back("Xylophone");
	gm_program_name.push_back("Tubular Bell");
	gm_program_name.push_back("Dulcimer");
	gm_program_name.push_back("Drawbar Organ");
	gm_program_name.push_back("Percussive Organ");
	gm_program_name.push_back("Rock Organ");
	gm_program_name.push_back("Church organ");
	gm_program_name.push_back("Reed organ");
	gm_program_name.push_back("Accordion");
	gm_program_name.push_back("Harmonica");
	gm_program_name.push_back("Tango Accordion");
	gm_program_name.push_back("Acoustic Guitar (nylon)");
	gm_program_name.push_back("Acoustic Guitar (steel)");
	gm_program_name.push_back("Electric Guitar (jazz)");
	gm_program_name.push_back("Electric Guitar (clean)");
	gm_program_name.push_back("Electric Guitar (muted)");
	gm_program_name.push_back("Overdriven Guitar");
	gm_program_name.push_back("Distortion Guitar");
	gm_program_name.push_back("Guitar harmonics");
	gm_program_name.push_back("Acoustic Bass");
	gm_program_name.push_back("Electric Bass (finger)");
	gm_program_name.push_back("Electric Bass (pick)");
	gm_program_name.push_back("Fretless Bass");
	gm_program_name.push_back("Slap Bass 1");
	gm_program_name.push_back("Slap Bass 2");
	gm_program_name.push_back("Synth Bass 1");
	gm_program_name.push_back("Synth Bass 2");
	gm_program_name.push_back("Violin");
	gm_program_name.push_back("Viola");
	gm_program_name.push_back("Cello");
	gm_program_name.push_back("Double bass");
	gm_program_name.push_back("Tremolo Strings");
	gm_program_name.push_back("Pizzicato Strings");
	gm_program_name.push_back("Orchestral Harp");
	gm_program_name.push_back("Timpani");
	gm_program_name.push_back("String Ensemble 1");
	gm_program_name.push_back("String Ensemble 2");
	gm_program_name.push_back("Synth Strings 1");
	gm_program_name.push_back("Synth Strings 2");
	gm_program_name.push_back("Voice Aahs");
	gm_program_name.push_back("Voice Oohs");
	gm_program_name.push_back("Synth Voice");
	gm_program_name.push_back("Orchestra Hit");
	gm_program_name.push_back("Trumpet");
	gm_program_name.push_back("Trombone");
	gm_program_name.push_back("Tuba");
	gm_program_name.push_back("Muted Trumpet");
	gm_program_name.push_back("French horn");
	gm_program_name.push_back("Brass Section");
	gm_program_name.push_back("Synth Brass 1");
	gm_program_name.push_back("Synth Brass 2");
	gm_program_name.push_back("Soprano Sax");
	gm_program_name.push_back("Alto Sax");
	gm_program_name.push_back("Tenor Sax");
	gm_program_name.push_back("Baritone Sax");
	gm_program_name.push_back("Oboe");
	gm_program_name.push_back("English Horn");
	gm_program_name.push_back("Bassoon");
	gm_program_name.push_back("Clarinet");
	gm_program_name.push_back("Piccolo");
	gm_program_name.push_back("Flute");
	gm_program_name.push_back("Recorder");
	gm_program_name.push_back("Pan Flute");
	gm_program_name.push_back("Blown Bottle");
	gm_program_name.push_back("Shakuhachi");
	gm_program_name.push_back("Whistle");
	gm_program_name.push_back("Ocarina");
	gm_program_name.push_back("Lead 1 (square)");
	gm_program_name.push_back("Lead 2 (sawtooth)");
	gm_program_name.push_back("Lead 3 (calliope)");
	gm_program_name.push_back("Lead 4 (chiff)");
	gm_program_name.push_back("Lead 5 (charang)");
	gm_program_name.push_back("Lead 6 (voice)");
	gm_program_name.push_back("Lead 7 (fifths)");
	gm_program_name.push_back("Lead 8 (bass + lead)");
	gm_program_name.push_back("Pad 1 (Fantasia)");
	gm_program_name.push_back("Pad 2 (warm)");
	gm_program_name.push_back("Pad 3 (polysynth)");
	gm_program_name.push_back("Pad 4 (choir)");
	gm_program_name.push_back("Pad 5 (bowed)");
	gm_program_name.push_back("Pad 6 (metallic)");
	gm_program_name.push_back("Pad 7 (halo)");
	gm_program_name.push_back("Pad 8 (sweep)");
	gm_program_name.push_back("FX 1 (rain)");
	gm_program_name.push_back("FX 2 (soundtrack)");
	gm_program_name.push_back("FX 3 (crystal)");
	gm_program_name.push_back("FX 4 (atmosphere)");
	gm_program_name.push_back("FX 5 (brightness)");
	gm_program_name.push_back("FX 6 (goblins)");
	gm_program_name.push_back("FX 7 (echoes)");
	gm_program_name.push_back("FX 8 (sci-fi)");
	gm_program_name.push_back("Sitar");
	gm_program_name.push_back("Banjo");
	gm_program_name.push_back("Shamisen");
	gm_program_name.push_back("Koto");
	gm_program_name.push_back("Kalimba");
	gm_program_name.push_back("Bagpipe");
	gm_program_name.push_back("Fiddle");
	gm_program_name.push_back("Shanai");
	gm_program_name.push_back("Tinkle Bell");
	gm_program_name.push_back("Agogo");
	gm_program_name.push_back("Steel Drums");
	gm_program_name.push_back("Woodblock");
	gm_program_name.push_back("Taiko Drum");
	gm_program_name.push_back("Melodic Tom");
	gm_program_name.push_back("Synth Drum");
	gm_program_name.push_back("Reverse Cymbal");
	gm_program_name.push_back("Guitar Fret Noise");
	gm_program_name.push_back("Breath Noise");
	gm_program_name.push_back("Seashore");
	gm_program_name.push_back("Bird Tweet");
	gm_program_name.push_back("Telephone Ring");
	gm_program_name.push_back("Helicopter");
	gm_program_name.push_back("Applause");
	gm_program_name.push_back("Gunshot");

	// die Event-Schleife
	while(run)
	{
		// MidiOut
		if(oldstep!=aktstep)
		{
			anzeige=true;
		}

		wstimer++;
		if(wstimer>20000)
		{
			zeitstempel = time(0);
			now = localtime(&zeitstempel);
			strftime (aktzeit,80,"%d.%m.%Y %X",now);

			wstimer = 0;
			anzeige = true;
		}

		blinktimer++;
		if(blinktimer>100000)
		{
			blinktimer = 0;
			if(blink==true)
				blink = false;
			else
				blink = true;
		}


		if(anzeige==true)
		{
			SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));

			// Arbeitsblatt
			boxColor(screen, scorex+0,1*scorey,40*scorex,22*scorey,0xBFBFBFFF);
			boxColor(screen, scorex+1,1*scorey+1,40*scorex-2,22*scorey-1,0x2F2F2FFF);

			// Device Tabs
			for(int i = 0;i<8;i++)
			{
				boxColor(screen, 2*scorex+(i+1)*2*scorex,0*scorey,2*scorex+2*(i+2)*scorex,1*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%d",i+1);
				if(seldev==i)
				{
					boxColor(screen, 2*scorex+(i+1)*2*scorex+1,0*scorey+1,2*scorex+2*(i+2)*scorex-1,1*scorey,0x2F2F2FFF);
					text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				}
				else
				{
					boxColor(screen, 2*scorex+(i+1)*2*scorex+1,0*scorey+1,2*scorex+2*(i+2)*scorex-1,1*scorey-1,0x1F1F1FFF);
					text = TTF_RenderText_Blended(font, temptext, textColor);
				}
				textPosition.x =2*scorex+(i+1)*2*scorex+1*scorex-text->w/2;
				textPosition.y = 0*scorey+2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
			}

			boxColor(screen, scorex+0*3*scorex,0*scorey,scorex+3*(0+1)*scorex,1*scorey,0xBFBFBFFF);
			SDL_FreeSurface(text);
			sprintf(temptext, "%s","Song");
			if(seldev==8)
			{
				boxColor(screen, scorex+0*3*scorex+1,0*scorey+1,scorex+3*(0+1)*scorex-1,1*scorey,0x2F2F2FFF);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
			}
			else
			{
				boxColor(screen, scorex+0*3*scorex+1,0*scorey+1,scorex+3*(0+1)*scorex-1,1*scorey-1,0x1F1F1FFF);
				text = TTF_RenderText_Blended(font, temptext, textColor);
			}
			textPosition.x = scorex+0*3*scorex+1.5*scorex-text->w/2;
			textPosition.y = 0*scorey+2;
			SDL_BlitSurface(text, 0, screen, &textPosition);

			boxColor(screen, scorex+8*4*scorex,0*scorey,scorex+4*(8+1)*scorex,1*scorey,0xBFBFBFFF);
			SDL_FreeSurface(text);
			sprintf(temptext, "%s","MSettings");
			if(seldev==9)
			{
				boxColor(screen, scorex+8*4*scorex+1,0*scorey+1,scorex+4*(8+1)*scorex-1,1*scorey,0x2F2F2FFF);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
			}
			else
			{
				boxColor(screen, scorex+8*4*scorex+1,0*scorey+1,scorex+4*(8+1)*scorex-1,1*scorey-1,0x1F1F1FFF);
				text = TTF_RenderText_Blended(font, temptext, textColor);
			}
			textPosition.x = scorex+8*4*scorex+2*scorex-text->w/2;
			textPosition.y = 0*scorey+2;
			SDL_BlitSurface(text, 0, screen, &textPosition);

			boxColor(screen, scorex+7*4*scorex,0*scorey,scorex+4*(7+1)*scorex,1*scorey,0xBFBFBFFF);
			SDL_FreeSurface(text);
			sprintf(temptext, "%s","DSettings");
			if(seldev==15)
			{
				boxColor(screen, scorex+7*4*scorex+1,0*scorey+1,scorex+4*(7+1)*scorex-1,1*scorey,0x2F2F2FFF);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
			}
			else
			{
				boxColor(screen, scorex+7*4*scorex+1,0*scorey+1,scorex+4*(7+1)*scorex-1,1*scorey-1,0x1F1F1FFF);
				text = TTF_RenderText_Blended(font, temptext, textColor);
			}
			textPosition.x = scorex+7*4*scorex+2*scorex-text->w/2;
			textPosition.y = 0*scorey+2;
			SDL_BlitSurface(text, 0, screen, &textPosition);

			boxColor(screen, scorex+6*4*scorex,0*scorey,scorex+4*(6+1)*scorex,1*scorey,0xBFBFBFFF);
			SDL_FreeSurface(text);
			sprintf(temptext, "%s","FluidSynth");
			if(seldev==16)
			{
				boxColor(screen, scorex+6*4*scorex+1,0*scorey+1,scorex+4*(6+1)*scorex-1,1*scorey,0x2F2F2FFF);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
			}
			else
			{
				boxColor(screen, scorex+6*4*scorex+1,0*scorey+1,scorex+4*(6+1)*scorex-1,1*scorey-1,0x1F1F1FFF);
				text = TTF_RenderText_Blended(font, temptext, textColor);
			}
			textPosition.x = scorex+6*4*scorex+2*scorex-text->w/2;
			textPosition.y = 0*scorey+2;
			SDL_BlitSurface(text, 0, screen, &textPosition);

			boxColor(screen, scorex+5*4*scorex,0*scorey,scorex+4*(5+1)*scorex,1*scorey,0xBFBFBFFF);
			SDL_FreeSurface(text);
			sprintf(temptext, "%s","Sampler");
			if(seldev==17)
			{
				boxColor(screen, scorex+5*4*scorex+1,0*scorey+1,scorex+4*(5+1)*scorex-1,1*scorey,0x2F2F2FFF);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
			}
			else
			{
				boxColor(screen, scorex+5*4*scorex+1,0*scorey+1,scorex+4*(5+1)*scorex-1,1*scorey-1,0x1F1F1FFF);
				text = TTF_RenderText_Blended(font, temptext, textColor);
			}
			textPosition.x = scorex+5*4*scorex+2*scorex-text->w/2;
			textPosition.y = 0*scorey+2;
			SDL_BlitSurface(text, 0, screen, &textPosition);

			boxColor(screen, scorex+9*4*scorex,0*scorey,4*scorex+4*(8+1)*scorex,1*scorey,0xBFBFBFFF);
			SDL_FreeSurface(text);
			sprintf(temptext, "%s","Info");
			if(seldev==10)
			{
				boxColor(screen, scorex+9*4*scorex+1,0*scorey+1,4*scorex+4*(8+1)*scorex-2,1*scorey,0x2F2F2FFF);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
			}
			else
			{
				boxColor(screen, scorex+9*4*scorex+1,0*scorey+1,4*scorex+4*(8+1)*scorex-2,1*scorey-1,0x1F1F1FFF);
				text = TTF_RenderText_Blended(font, temptext, textColor);
			}
			textPosition.x = scorex+9*4*scorex+1.5*scorex-text->w/2;
			textPosition.y = 0*scorey+2;
			SDL_BlitSurface(text, 0, screen, &textPosition);

			// MIDI IN Anzeige
			if(seldev<9)
			{
				sprintf(temptext, "%s","MIDI IN");
				if(midiindata)
				{
					text = TTF_RenderText_Blended(fontbold, temptext, {0x3F,0xFF,0x3F});
				}
				else
				{
					text = TTF_RenderText_Blended(fontbold, temptext, {0x3F,0x3F,0x3F});
				}
				textPosition.x = 2*scorex;
				textPosition.y = 3*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				sprintf(temptext, "%s","MIDI CLOCK");
				if(midiinclockdata)
				{
					text = TTF_RenderText_Blended(fontbold, temptext, {0x3F,0xFF,0x3F});
				}
				else
				{
					text = TTF_RenderText_Blended(fontbold, temptext, {0x3F,0x3F,0x3F});
				}
				textPosition.x = 2*scorex;
				textPosition.y = 4*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
			}

			lineColor(screen, 14*scorex-1, 22*scorey+1, 14*scorex-1, 24*scorey,0x2F2F2FFF);
			lineColor(screen, 14*scorex, 22*scorey+1, 14*scorex, 24*scorey,0x8F8F8FFF);
			lineColor(screen, 14*scorex+1, 22*scorey+1, 14*scorex+1, 24*scorey,0x2F2F2FFF);

			// CPU und RAM
			boxColor(screen, 40.2*scorex,0*scorey,40.4*scorex,1*scorey,0x2F2F2FFF);
			if(cpuusage>90)
			{
				boxColor(screen, 40.2*scorex,(0+(100-cpuusage)/100)*scorey,40.4*scorex,1*scorey,0xFF0000FF);
			}
			else if(cpuusage>80)
			{
				boxColor(screen, 40.2*scorex,(0+(100-cpuusage)/100)*scorey,40.4*scorex,1*scorey,0xFFFF00FF);
			}
			else
			{
				boxColor(screen, 40.2*scorex,(0+(100-cpuusage)/100)*scorey,40.4*scorex,1*scorey,0x00FF00FF);
			}

			boxColor(screen, 40.6*scorex,0*scorey,40.8*scorex,1*scorey,0x2F2F2FFF);
			if(float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram)<0.1)
			{
				boxColor(screen, 40.6*scorex,(0+float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram))*scorey,40.8*scorex,1*scorey,0xFF0000FF);
			}
			else if(float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram)<0.2)
			{
				boxColor(screen, 40.6*scorex,(0+float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram))*scorey,40.8*scorex,1*scorey,0xFFFF00FF);
			}
			else
			{
				boxColor(screen, 40.6*scorex,(0+float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram))*scorey,40.8*scorex,1*scorey,0x00FF00FF);
			}

			// Time
			SDL_FreeSurface(text);
			text = TTF_RenderText_Blended(font, aktzeit, textColor);
			textPosition.x = 20*scorex-text->w/2;
			textPosition.y = 21*scorey+text->h/3;
			SDL_BlitSurface(text, 0, screen, &textPosition);

			// Arbeitsblatt MidiSettings
			if(seldev==9)
			{
				// Save Config
				save_button.x = 6*scorex;
				save_button.y = 22*scorey+2;
				SDL_BlitSurface(save_image, 0, screen, &save_button);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "MIDI Settings", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 2*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				settings_miditype_rect.push_back(textPosition);

				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Name");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 6*scorex;
				textPosition.y = 3*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(temptext, "%s","MIDI-Device");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 15*scorex;
				textPosition.y = 3*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(temptext, "%s","MCh#");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 34*scorex;
				textPosition.y = 3*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Type");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 37*scorex;
				textPosition.y = 3*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				settings_devname_rect.clear();
				settings_mididevice_rect.clear();
				settings_midichannel_rect.clear();
				settings_miditype_rect.clear();

				for(int i=0;i<8;i++)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "Device %d",i+1);
					text = TTF_RenderText_Blended(fontbold, temptext, textColor);
					textPosition.x = 2*scorex;
					textPosition.y = (i+4)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "%s",dsettings[i].name);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 6*scorex;
					textPosition.y = (i+4)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					settings_devname_rect.push_back(textPosition);

					SDL_FreeSurface(text);
					if(int(dsettings[i].mididevice)==255)
					{
						sprintf(temptext, "%s","FluidSynth");
					}
					else if(int(dsettings[i].mididevice)==254)
					{
						sprintf(temptext, "%s","Sampler");
					}
					else if(int(dsettings[i].mididevice)<onPorts)
					{
						sprintf(temptext, "%s",midioutname[dsettings[i].mididevice].c_str());
					}
					else
					{
						sprintf(temptext, "%s","Mididevice not available");
					}
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 15*scorex;
					textPosition.y = (i+4)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					settings_mididevice_rect.push_back(textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "%d",dsettings[i].midichannel);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 34*scorex;
					textPosition.y = (i+4)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					settings_midichannel_rect.push_back(textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "%s",dsettings[i].type);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 37*scorex;
					textPosition.y = (i+4)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					settings_miditype_rect.push_back(textPosition);
				}

				// Midiin
				SDL_FreeSurface(text);
				sprintf(temptext, "%s",midiindevname);
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 6*scorex;
				textPosition.y = 13*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				settings_midiindevname_rect = textPosition;

				SDL_FreeSurface(text);
				sprintf(temptext, "%s","MIDI In");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 2*scorex;
				textPosition.y =13*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				if(midiindevice<inPorts)
				{
					sprintf(temptext, "%s",midiinname[midiindevice].c_str());
				}
				else
				{
					sprintf(temptext, "%s","Mididevice not available");
				}
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 15*scorex;
				textPosition.y = 13*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				midiindev_rect = textPosition;

				SDL_FreeSurface(text);
				sprintf(temptext, "%d",midiinch+1);
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 34*scorex;
				textPosition.y =13*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				midiinch_rect = textPosition;

				SDL_FreeSurface(text);
				sprintf(temptext, "%s","MIDI Clock");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 2*scorex;
				textPosition.y =14*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				if(midiinclockdevice<inPorts)
				{
					sprintf(temptext, "%s",midiinname[midiinclockdevice].c_str());
				}
				else
				{
					sprintf(temptext, "%s","Mididevice not available");
				}
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 15*scorex;
				textPosition.y = 14*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				midiinclockdev_rect = textPosition;

			}

			// Arbeitsblatt Drum Settings
			else if(seldev==15)
			{
				// Save Config
				save_button.x = 6*scorex;
				save_button.y = 22*scorey+2;
				SDL_BlitSurface(save_image, 0, screen, &save_button);

				// Drumsettings
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Drum Settings", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 2*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Name", textColor);
				textPosition.x = 6*scorex;
				textPosition.y = 3*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Note", textColor);
				textPosition.x = 15*scorex;
				textPosition.y = 3*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				for(int i=0;i<16;i++)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "%s",drumsettings[i].name);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 6*scorex;
					textPosition.y = (i+4)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					settings_miditype_rect.push_back(textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "%d",drumsettings[i].note);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 15*scorex;
					textPosition.y = (i+4)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					settings_miditype_rect.push_back(textPosition);
				}
			}

			// Arbeitsblatt FluidSynth
			else if(seldev==16)
			{
				// FluidSynthsettings
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "FluidSynth Settings", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 2*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

//			    cout << "MIDI-Channels: " << fluid_nmid_chan << endl;
//			    cout << "Audio ALSA-Device: " << fluid_alsa_device << endl;

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "MIDI-Channels", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 4*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(temptext, "%d",fluid_nmid_chan);
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 12*scorex;
				textPosition.y = 4*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Audio ALSA-Device", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 5*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(temptext, "%s",fluid_alsa_device);
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 12*scorex;
				textPosition.y = 5*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "CPU Load", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 6*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(temptext, "%d",int(fluid_synth_get_cpu_load(fluid_synth)));
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 12*scorex;
				textPosition.y = 6*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "MCh", textColor);
				textPosition.x = 22*scorex;
				textPosition.y = 4*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				settings_miditype_rect.push_back(textPosition);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Bank", textColor);
				textPosition.x = 24*scorex;
				textPosition.y = 4*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				settings_miditype_rect.push_back(textPosition);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Prog", textColor);
				textPosition.x = 26*scorex;
				textPosition.y = 4*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				settings_miditype_rect.push_back(textPosition);

				int i = 0;
				fluid_settings_bank_rect.clear();
				fluid_settings_program_rect.clear();
			    for(auto &fptmp: fluid_program_state)
			    {
					SDL_FreeSurface(text);
					sprintf(temptext, "%d",fptmp.channel+1);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 23*scorex-text->w/2;
					textPosition.y = (i+5)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "%d",fptmp.bank);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 25*scorex-text->w/2;
					textPosition.y = (i+5)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					fluid_settings_bank_rect.push_back(textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "%d",fptmp.program);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 27*scorex-text->w/2;
					textPosition.y = (i+5)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					fluid_settings_program_rect.push_back(textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "%s",gm_program_name[fptmp.program]);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 29*scorex;
					textPosition.y = (i+5)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					fluid_settings_program_rect.push_back(textPosition);

			    	i++;
			    }

			}

			// Arbeitsblatt Sampler
			else if(seldev==17)
			{
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Sampleplayer");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 2*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				for(int i=0;i<16;i++)
				{
					if(selkey==i)
					{
						boxColor(screen,4*scorex,(5+i)*scorey, 20*scorex, (6+i)*scorey,0xCFCFCFFF);
					}
				}

				for(int i=0;i<16;i++)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "%d",i+16*(oktave[seldev]+2));
					text = TTF_RenderText_Blended(fontbold, temptext, textColor);
					textPosition.x = 3*scorex-text->w;
					textPosition.y = (i+5.5)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
				}
				
				
				for(int i=5;i<22;i++)
				{
					lineColor(screen, 4*scorex, i*scorey, 20*scorex, i*scorey,0xBFBFBFFF);
				}
				lineColor(screen, 4*scorex, 5*scorey, 4*scorex, 21*scorey,0xBFBFBFFF);
				lineColor(screen, 20*scorex, 5*scorey, 20*scorex, 21*scorey,0xBFBFBFFF);

				// Samples
				for(int i=0;i<16;i++)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "%s"," ");
					if(strcmp(sndbnk[i+(oktave[seldev]+2)*16].name,"X")!=0)
					{
						sprintf(temptext, "%s",sndbnk[i+(oktave[seldev]+2)*16].name);
					}
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x =4.1*scorex;
					textPosition.y = (i+5.5)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
				}
				for(int i=0;i<16;i++)
				{
					if(strcmp(sndbnk[i+(oktave[seldev]+2)*16].name,"X")!=0)
					{
						opensample_button[i].x = 20.1*scorex;
						opensample_button[i].y = (i+5)*scorey;
						SDL_BlitSurface(sampledelete_image, 0, screen, &opensample_button[i]);
					}
					else
					{
						opensample_button[i].x = 20.1*scorex;
						opensample_button[i].y = (i+5)*scorey;
						SDL_BlitSurface(opensample_image, 0, screen, &opensample_button[i]);
					}
				}
				up_button.x = 0;
				up_button.y = 5*scorey;
				SDL_BlitSurface(up_image, 0, screen, &up_button);
				down_button.x = 0;
				down_button.y = 16*scorey;
				SDL_BlitSurface(down_image, 0, screen, &down_button);
			}

			// Arbeitsblatt Lösche Sample
			else if(seldev==18)
			{
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Delete Sample");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y =3.5*scorey-text->h/2;

				boxColor(screen, 12*scorex,3*scorey,28*scorex,7.5*scorey,0xFFFFFFFF);
				boxColor(screen, 12*scorex+1,3*scorey+1,28*scorex-1,7.5*scorey-1,0x2F2F2FFF);
				lineColor(screen, 12*scorex, 4*scorey, 28*scorex, 4*scorey,0xFFFFFFFF);
				SDL_BlitSurface(text, 0, screen, &textPosition);
				load_window.x = 12*scorex;
				load_window.y = 4*scorey;
				load_window.w = 16*scorex;
				load_window.h = 15*scorey;

				boxColor(screen, 14*scorex,6.25*scorey,18*scorex,7.25*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Ok");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 16*scorex-text->w/2;
				textPosition.y =6.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				ok_button.x = 14*scorex;
				ok_button.y = 6.25*scorey;
				ok_button.w = 4*scorex;
				ok_button.h = scorey;

				boxColor(screen, 22*scorex,6.25*scorey,26*scorex,7.25*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Cancel");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 24*scorex-text->w/2;
				textPosition.y =6.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				cancel_button.x = 22*scorex;
				cancel_button.y = 6.25*scorey;
				cancel_button.w = 4*scorex;
				cancel_button.h = scorey;

				lineColor(screen, 12*scorex, 6*scorey, 28*scorex, 6*scorey,0xBFBFBFFF);

				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Really delete Sample?");
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 20*scorex-text->w/2;
				textPosition.y = 5*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
			}

			// Arbeitsblatt Load Samples
			else if(seldev==19)
			{
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Load Sample");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y =3.5*scorey-text->h/2;

				boxColor(screen, 12*scorex,3*scorey,28*scorex,21*scorey,0xFFFFFFFF);
				boxColor(screen, 12*scorex+1,3*scorey+1,28*scorex-1,21*scorey-1,0x2F2F2FFF);
				lineColor(screen, 12*scorex, 4*scorey, 28*scorex, 4*scorey,0xFFFFFFFF);
				SDL_BlitSurface(text, 0, screen, &textPosition);
				load_window.x = 12*scorex;
				load_window.y = 4*scorey;
				load_window.w = 16*scorex;
				load_window.h = 15*scorey;

				boxColor(screen, 14*scorex,19.75*scorey,18*scorex,20.75*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Ok");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 16*scorex-text->w/2;
				textPosition.y =20.25*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				ok_button.x = 14*scorex;
				ok_button.y = 19.75*scorey;
				ok_button.w = 4*scorex;
				ok_button.h = scorey;

				boxColor(screen, 22*scorex,19.75*scorey,26*scorex,20.75*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Cancel");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 24*scorex-text->w/2;
				textPosition.y =20.25*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				cancel_button.x = 22*scorex;
				cancel_button.y = 19.75*scorey;
				cancel_button.w = 4*scorex;
				cancel_button.h = scorey;

				lineColor(screen, 12*scorex, 19.5*scorey, 28*scorex, 19.5*scorey,0xBFBFBFFF);

				SDL_BlitSurface(text, 0, screen, &textPosition);

				if(startdir>0)
				{
					up_button.x = 28*scorex;
					up_button.y = 4*scorey;
					SDL_BlitSurface(up_image, 0, screen, &up_button);
				}

				down_button.x = 28*scorex;
				down_button.y = 18*scorey;
				SDL_BlitSurface(down_image, 0, screen, &down_button);

				int i=0;
				anzdir=0;
				for(auto samplename: sampledir)
				{
					if(anzdir>=startdir and anzdir-startdir<maxdir)
					{
						SDL_FreeSurface(text);
						sprintf(temptext, "%s",samplename.c_str());
						text = TTF_RenderText_Blended(font, temptext, textColor);
						textPosition.x = 12.5*scorex;
						textPosition.y = (4.5+i)*scorey-text->h/2;
						SDL_BlitSurface(text, 0, screen, &textPosition);
						i++;
					}
					anzdir++;
				}
				if(selloadsample<255)
				{
					boxColor(screen, 12*scorex+1,(4+selloadsample)*scorey+1, 28*scorex-1, (5+selloadsample)*scorey-1,0xBF2F2F88);
				}
			}
			// Arbeitsblatt Info
			else if(seldev==10)
			{
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Sequencegang - a sequencer for Linux/SDL");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 2*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Version 4.0");
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 3*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","(c) 1987-2019 by Wolfgang Schuster");
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 4*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","GNU General Public License v3.0");
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 5*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(temptext, "%s","MIDI Out");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 7*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				int i = 0;
				for(auto &mout: midioutname)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "%s",mout.c_str());
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 2*scorex;
					textPosition.y = (8+i)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					i++;
				}

				i = 0;
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","MIDI In");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 20*scorex;
				textPosition.y =(7+i)*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				for(auto &min: midiinname)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "%s",min.c_str());
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 20*scorex;
					textPosition.y = (8+i)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					i++;
				}

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "CPU usage", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 18*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				SDL_FreeSurface(text);
				sprintf(temptext, "%f",cpuusage);
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 10*scorex;
				textPosition.y = 18*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "MEM free %", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 20*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				SDL_FreeSurface(text);
				sprintf(temptext, "%lf %",float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram)*100);
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 10*scorex;
				textPosition.y = 20*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				text = TTF_RenderText_Blended(fontbold, "MEM free MB", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 21*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				SDL_FreeSurface(text);
				sprintf(temptext, "%ld",memInfo.freeram/1024);
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 10*scorex;
				textPosition.y = 21*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
			}

			// Arbeitsblatt Exit
			else if(seldev==13)
			{
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Exit");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y =3.5*scorey-text->h/2;

				boxColor(screen, 12*scorex,3*scorey,28*scorex,7.5*scorey,0xFFFFFFFF);
				boxColor(screen, 12*scorex+1,3*scorey+1,28*scorex-1,7.5*scorey-1,0x2F2F2FFF);
				lineColor(screen, 12*scorex, 4*scorey, 28*scorex, 4*scorey,0xFFFFFFFF);
				SDL_BlitSurface(text, 0, screen, &textPosition);
				load_window.x = 12*scorex;
				load_window.y = 4*scorey;
				load_window.w = 16*scorex;
				load_window.h = 15*scorey;

				boxColor(screen, 14*scorex,6.25*scorey,18*scorex,7.25*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Ok");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 16*scorex-text->w/2;
				textPosition.y =6.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				ok_button.x = 14*scorex;
				ok_button.y = 6.25*scorey;
				ok_button.w = 4*scorex;
				ok_button.h = scorey;

				boxColor(screen, 22*scorex,6.25*scorey,26*scorex,7.25*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Cancel");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 24*scorex-text->w/2;
				textPosition.y =6.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				cancel_button.x = 22*scorex;
				cancel_button.y = 6.25*scorey;
				cancel_button.w = 4*scorex;
				cancel_button.h = scorey;

				lineColor(screen, 12*scorex, 6*scorey, 28*scorex, 6*scorey,0xBFBFBFFF);

				SDL_FreeSurface(text);
				if(raspi==true)
				{
					sprintf(temptext, "%s","Really Shutdown?");
				}
				else
				{
					sprintf(temptext, "%s","Really Exit Program?");
				}
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 20*scorex-text->w/2;
				textPosition.y = 5*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
			}
			// Arbeitsblatt Load Sequences
			else if(seldev==11)
			{
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Load Song");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y =3.5*scorey-text->h/2;

				boxColor(screen, 12*scorex,3*scorey,28*scorex,21*scorey,0xFFFFFFFF);
				boxColor(screen, 12*scorex+1,3*scorey+1,28*scorex-1,21*scorey-1,0x2F2F2FFF);
				lineColor(screen, 12*scorex, 4*scorey, 28*scorex, 4*scorey,0xFFFFFFFF);
				SDL_BlitSurface(text, 0, screen, &textPosition);
				load_window.x = 12*scorex;
				load_window.y = 4*scorey;
				load_window.w = 16*scorex;
				load_window.h = 15*scorey;

				boxColor(screen, 14*scorex,19.75*scorey,18*scorex,20.75*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Ok");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 16*scorex-text->w/2;
				textPosition.y =20.25*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				ok_button.x = 14*scorex;
				ok_button.y = 19.75*scorey;
				ok_button.w = 4*scorex;
				ok_button.h = scorey;

				boxColor(screen, 22*scorex,19.75*scorey,26*scorex,20.75*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Cancel");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 24*scorex-text->w/2;
				textPosition.y =20.25*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				cancel_button.x = 22*scorex;
				cancel_button.y = 19.75*scorey;
				cancel_button.w = 4*scorex;
				cancel_button.h = scorey;

				lineColor(screen, 12*scorex, 19.5*scorey, 28*scorex, 19.5*scorey,0xBFBFBFFF);

				SDL_BlitSurface(text, 0, screen, &textPosition);

				if(startdir>0)
				{
					up_button.x = 28*scorex;
					up_button.y = 4*scorey;
					SDL_BlitSurface(up_image, 0, screen, &up_button);
				}

				down_button.x = 28*scorex;
				down_button.y = 18*scorey;
				SDL_BlitSurface(down_image, 0, screen, &down_button);

				int i=0;
				anzdir=0;
				for(auto songname: songdir)
				{
					if(anzdir>=startdir and anzdir-startdir<maxdir)
					{
						SDL_FreeSurface(text);
						sprintf(temptext, "%s",songname.c_str());
						text = TTF_RenderText_Blended(font, temptext, textColor);
						textPosition.x = 12.5*scorex;
						textPosition.y = (4.5+i)*scorey-text->h/2;
						SDL_BlitSurface(text, 0, screen, &textPosition);
						i++;
					}
					anzdir++;
				}
				if(selloadsong<255)
				{
					boxColor(screen, 12*scorex+1,(4+selloadsong)*scorey+1, 28*scorex-1, (5+selloadsong)*scorey-1,0xBF2F2F88);
				}



			}
			// Arbeitsblatt Save
			else if(seldev==12)
			{
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Save Song");
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y =3.5*scorey-text->h/2;

				boxColor(screen, 12*scorex,3*scorey,28*scorex,7*scorey,0xFFFFFFFF);
				boxColor(screen, 12*scorex+1,3*scorey+1,28*scorex-1,7*scorey-1,0x2F2F2FFF);
				lineColor(screen, 12*scorex, 4*scorey, 28*scorex, 4*scorey,0xFFFFFFFF);
				SDL_BlitSurface(text, 0, screen, &textPosition);

				boxColor(screen, 14*scorex,5.75*scorey,18*scorex,6.75*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Ok");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 16*scorex-text->w/2;
				textPosition.y = 6.25*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				ok_button.x = 14*scorex;
				ok_button.y = 5.75*scorey;
				ok_button.w = 4*scorex;
				ok_button.h = scorey;

				boxColor(screen, 22*scorex,5.75*scorey,26*scorex,6.75*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Cancel");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 24*scorex-text->w/2;
				textPosition.y = 6.25*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				cancel_button.x = 22*scorex;
				cancel_button.y = 5.75*scorey;
				cancel_button.w = 4*scorex;
				cancel_button.h = scorey;

				SDL_FreeSurface(text);
				if(blink==true)
				{
					sprintf(temptext, "%s_",songname);
				}
				else
				{
					if(string(songname).length()>0)
					{
						sprintf(temptext, "%s",songname);
					}
					else
					{
						sprintf(temptext, "%s"," ");
					}
				}
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 12.5*scorex;
				textPosition.y = 4.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				lineColor(screen, 12*scorex, 5.5*scorey, 28*scorex, 5.5*scorey,0xBFBFBFFF);
			}

			// Arbeitsblatt Texteingabe
			else if(seldev==14)
			{
				SDL_FreeSurface(text);
				sprintf(temptext, "%s",textinputtitel);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y =3.5*scorey-text->h/2;

				boxColor(screen, 12*scorex,3*scorey,28*scorex,7*scorey,0xFFFFFFFF);
				boxColor(screen, 12*scorex+1,3*scorey+1,28*scorex-1,7*scorey-1,0x2F2F2FFF);
				lineColor(screen, 12*scorex, 4*scorey, 28*scorex, 4*scorey,0xFFFFFFFF);
				SDL_BlitSurface(text, 0, screen, &textPosition);

				boxColor(screen, 14*scorex,5.75*scorey,18*scorex,6.75*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Ok");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 16*scorex-text->w/2;
				textPosition.y = 6.25*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				ok_button.x = 14*scorex;
				ok_button.y = 5.75*scorey;
				ok_button.w = 4*scorex;
				ok_button.h = scorey;

				boxColor(screen, 22*scorex,5.75*scorey,26*scorex,6.75*scorey,0xBFBFBFFF);
				SDL_FreeSurface(text);
				sprintf(temptext, "%s","Cancel");
				text = TTF_RenderText_Blended(fontbold, temptext, blackColor);
				textPosition.x = 24*scorex-text->w/2;
				textPosition.y = 6.25*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				cancel_button.x = 22*scorex;
				cancel_button.y = 5.75*scorey;
				cancel_button.w = 4*scorex;
				cancel_button.h = scorey;

				SDL_FreeSurface(text);
				if(blink==true)
				{
					sprintf(temptext, "%s_",texteingabe);
				}
				else
				{
					if(string(texteingabe).length()>0)
					{
						sprintf(temptext, "%s",texteingabe);
					}
					else
					{
						sprintf(temptext, "%s"," ");
					}
				}
				text = TTF_RenderText_Blended(font, temptext, textColor);
				textPosition.x = 12.5*scorex;
				textPosition.y = 4.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);


				lineColor(screen, 12*scorex, 5.5*scorey, 28*scorex, 5.5*scorey,0xBFBFBFFF);
			}

			// Arbeitsblatt Song
			else if(seldev==8)
			{
				// Exit Program
				shutdown_button.x = 2*scorex;
				shutdown_button.y = 22*scorey+1;
				SDL_BlitSurface(shutdown_image, 0, screen, &shutdown_button);

				// Load Song
				load_button.x = 5*scorex;
				load_button.y = 22*scorey+1;
				SDL_BlitSurface(load_image, 0, screen, &load_button);

				// Save Song
				save_button.x = 8*scorex;
				save_button.y = 22*scorey+2;
				SDL_BlitSurface(save_image, 0, screen, &save_button);

				// Edit Song
				edit_button.x = 11*scorex;
				edit_button.y = 22*scorey+1;
				if(isedit==true)
				{
					SDL_BlitSurface(speaker_image, 0, screen, &edit_button);
				}
				else
				{
					SDL_BlitSurface(edit_image, 0, screen, &edit_button);
				}

				if(isedit==true)
				{
					if(selpatt[0]==8)
					{
						minus_button.x = 33*scorex;
						minus_button.y = 22*scorey+1;
						SDL_BlitSurface(minus_image, 0, screen, &minus_button);
						plus_button.x = 36*scorex;
						plus_button.y = 22*scorey+1;
						SDL_BlitSurface(plus_image, 0, screen, &plus_button);
					}

					if(selpatt[0]==8 and selpatt[3]==8)
					{
						ende_button.x = 27*scorex;
						ende_button.y = 22*scorey+1;
						SDL_BlitSurface(ende_image, 0, screen, &ende_button);
						repeat_button.x = 30*scorex;
						repeat_button.y = 22*scorey+1;
						SDL_BlitSurface(repeat_image, 0, screen, &repeat_button);
					}
				}
				else
				{
					// All Notes Off
					allnotesoff_button.x = 24*scorex;
					allnotesoff_button.y = 22*scorey+2;
					SDL_BlitSurface(allnotesoff_image, 0, screen, &allnotesoff_button);

					// Timersource
					plug_button.x = 21*scorex;
					plug_button.y = 22*scorey+2;
					SDL_BlitSurface(plug_image, 0, screen, &plug_button);

					// Play, Pause, Stop
					if(timerrun==true)
					{
						play_button.x = 30*scorex;
						play_button.y = 22*scorey+1;
						SDL_BlitSurface(pause_image, 0, screen, &play_button);
					}
					else
					{
						play_button.x = 30*scorex;
						play_button.y = 22*scorey+1;
						SDL_BlitSurface(play_image, 0, screen, &play_button);
					}

					stop_button.x = 27*scorex;
					stop_button.y = 22*scorey+1;
					SDL_BlitSurface(stop_image, 0, screen, &stop_button);

					// forward, backward
					backward_button.x = 33*scorex;
					backward_button.y = 22*scorey+1;
					SDL_BlitSurface(backward_image, 0, screen, &backward_button);
					forward_button.x = 36*scorex;
					forward_button.y = 22*scorey+1;
					SDL_BlitSurface(forward_image, 0, screen, &forward_button);
				}

				SDL_FreeSurface(text);
//				sprintf(temptext, "%s",songname);
				text = TTF_RenderText_Blended(fontbold, songname, textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				if(clockmodeext==true)
				{
					sprintf(temptext, "BPM: %s","ext");
				}
				else
				{
					sprintf(temptext, "BPM: %d",bpm);
				}
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 12*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				bpmrect = textPosition;

				SDL_FreeSurface(text);
				sprintf(temptext, "Time Division: 1/%d",timedivision);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 16*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				timedevrect = textPosition;

				SDL_FreeSurface(text);
				sprintf(temptext, "Steps: %d",maxstep+1);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 23*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				maxsteprect = textPosition;

				SDL_FreeSurface(text);
				if(aktstep==64)
				{
					sprintf(temptext, "Step: %d",0);
				}
				else
				{
					sprintf(temptext, "Step: %d",aktstep+1);
				}
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 28*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				if(songstep==64)
				{
					sprintf(temptext, "Songstep: %d",0);
				}
				else
				{
					sprintf(temptext, "Songstep: %d",songstep+1);
				}
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 31.5*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				if(selpatt[2]!=255 and selpatt[selpat[3]]!=255 and debug==true)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "selpatt: %d %d %d %d", selpatt[0], selpatt[1],selpatt[2]+startanzeigepattern*16,selpatt[3]);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 10*scorex;
					textPosition.y = 2.5*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "[%d][%d][%d]", pattern[selpatt[0]][selpatt[1]][selpatt[2]+startanzeigepattern*16][selpatt[3]][0], pattern[selpatt[0]][selpatt[1]][selpatt[2]+startanzeigepattern*16][selpatt[3]][1], pattern[selpatt[0]][selpatt[1]][selpatt[2]+startanzeigepattern*16][selpatt[3]][2]);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 16*scorex;
					textPosition.y = 2.5*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "[%d]", songrepeat);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 18*scorex;
					textPosition.y = 2.5*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);
				}

				// Devicenamen
				for(int i=0;i<8;i++)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "%s",dsettings[i].name);
					text = TTF_RenderText_Blended(fontbold, temptext, textColor);
					textPosition.x = 2*scorex;
					textPosition.y = (i+5.5)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					lineColor(screen, 12*scorex, (5+i)*scorey, 36*scorex, (5+i)*scorey,0x8F8F8FFF);

					mute_button[i].x = 10.75*scorex;
					mute_button[i].y = (i+5.5)*scorey-muted_image->h/2;
					if(ismuted[i]==false)
					{
						SDL_BlitSurface(notmuted_image, 0, screen, &mute_button[i]);
					}
					else
					{
						SDL_BlitSurface(muted_image, 0, screen, &mute_button[i]);
					}
				}
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Control / BPM", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = (8+5.5)*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				if(timerrun==true)
				{
					if(songstep<12)
					{
						songanzeigepattern=0;
					}
					else if(songstep<24)
					{
						songanzeigepattern=3;
					}
					else if(songstep<36)
					{
						songanzeigepattern=6;
					}
					else if(songstep<48)
					{
						songanzeigepattern=9;
					}
					else if(songstep<60)
					{
						songanzeigepattern=12;
					}
					else
					{
						songanzeigepattern=13;
					}
				}

				for(int i=0;i<12;i++)
				{
					if(songstep==i+songanzeigepattern*4)
					{
						boxColor(screen, (6+i)*2*scorex+1,4*scorey+1, (7+i)*2*scorex-1, 5*scorey-1,0x2FFF2F44);
					}
					SDL_FreeSurface(text);
					sprintf(temptext, "%d",i+1+songanzeigepattern*4);
					text = TTF_RenderText_Blended(fontsmall, temptext, textColor);
					textPosition.x = (6.5+i)*2*scorex-text->w/2;
					textPosition.y = 4.5*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					lineColor(screen, (6+i)*2*scorex, 4.2*scorey, (6+i)*2*scorex, 14*scorey,0x4F4F4FFF);
				}

				lineColor(screen, 12*scorex, 5*scorey, 36*scorex, 5*scorey,0xFFFFFFFF);
				lineColor(screen, 12*scorex, 13*scorey, 36*scorex, 13*scorey,0xFFFFFFFF);
				lineColor(screen, 12*scorex, 14*scorey, 36*scorex, 14*scorey,0xFFFFFFFF);
				lineColor(screen, 12*scorex, 4.2*scorey, 12*scorex, 14*scorey,0xFFFFFFFF);
				lineColor(screen, 20*scorex, 4.2*scorey, 20*scorex, 14*scorey,0xFFFFFFFF);
				lineColor(screen, 28*scorex, 4.2*scorey, 28*scorex, 14*scorey,0xFFFFFFFF);
				lineColor(screen, 36*scorex, 4.2*scorey, 36*scorex, 14*scorey,0xFFFFFFFF);

				for(int i=0;i<12;i++)
				{
					for(int j=0;j<9;j++)
					{
						if(pattern[8][songtab][i+songanzeigepattern*4][j][0]==1)
						{
							if(pattern[8][songtab][i+songanzeigepattern*4][j][1]==16)
							{
								boxColor(screen, (6+i)*2*scorex+1,(5+j)*scorey+1, (7+i)*2*scorex-1, (6+j)*scorey-1,0xFFFF2FFF);
								SDL_FreeSurface(text);
								text = TTF_RenderText_Blended(font, "ANO", blackColor);
								textPosition.x = (6.5+i)*2*scorex-text->w/2;
								textPosition.y = (5.5+j)*scorey-text->h/2;
								SDL_BlitSurface(text, 0, screen, &textPosition);
							}
							else
							{
								boxColor(screen, (6+i)*2*scorex+1,(5+j)*scorey+1, (7+i)*2*scorex-1, (6+j)*scorey-1,0x2FFF2FFF);
								SDL_FreeSurface(text);
								sprintf(temptext, "%d",pattern[8][songtab][i+songanzeigepattern*4][j][1]+1);
								text = TTF_RenderText_Blended(font, temptext, blackColor);
								textPosition.x = (6.5+i)*2*scorex-text->w/2;
								textPosition.y = (5.5+j)*scorey-text->h/2;
								SDL_BlitSurface(text, 0, screen, &textPosition);
							}
						}
						// Repeat
						if(pattern[8][songtab][i+songanzeigepattern*4][j][0]==4)
						{
							boxColor(screen, (6+i)*2*scorex+1,(5+j)*scorey+1, (7+i)*2*scorex-1, (6+j)*scorey-1,0x8F8FFFFF);
							SDL_FreeSurface(text);
							sprintf(temptext, "R: %d",pattern[8][songtab][i+songanzeigepattern*4][j][1]);
							text = TTF_RenderText_Blended(font, temptext , blackColor);
							textPosition.x = (6.5+i)*2*scorex-text->w/2;
							textPosition.y = (5.5+j)*scorey-text->h/2;
							SDL_BlitSurface(text, 0, screen, &textPosition);
						}
						// Ende
						if(pattern[8][songtab][i+songanzeigepattern*4][j][0]==3)
						{
							boxColor(screen, (6+i)*2*scorex+1,(5+j)*scorey+1, (7+i)*2*scorex-1, (6+j)*scorey-1,0xFF2F2FFF);
							SDL_FreeSurface(text);
							text = TTF_RenderText_Blended(fontbold, "END", textColor);
							textPosition.x = (6.5+i)*2*scorex-text->w/2;
							textPosition.y = (5.5+j)*scorey-text->h/2;
							SDL_BlitSurface(text, 0, screen, &textPosition);
						}
						// BPM
						if(pattern[8][songtab][i+songanzeigepattern*4][j][0]==2)
						{
							boxColor(screen, (6+i)*2*scorex+1,(5+j)*scorey+1, (7+i)*2*scorex-1, (6+j)*scorey-1,0x2F2FFFFF);
							SDL_FreeSurface(text);
							sprintf(temptext, "%d",pattern[8][songtab][i+songanzeigepattern*4][j][1]*4);
							text = TTF_RenderText_Blended(font, temptext, textColor);
							textPosition.x = (6.5+i)*2*scorex-text->w/2;
							textPosition.y = (5.5+j)*scorey-text->h/2;
							SDL_BlitSurface(text, 0, screen, &textPosition);
						}

						// Selected
						if(selpatt[2]-songanzeigepattern*4==i and selpatt[3]==j)
						{
							boxColor(screen, (6+i)*2*scorex+1,(5+j)*scorey+1, (7+i)*2*scorex-1, (6+j)*scorey-1,0xBF2F2F88);
						}
					}
				}

				// Patternleiste
				for(int j=0;j<8;j++)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "%d",j+1);
					if(songtab==j)
					{
						boxColor(screen, 0,(3+j)*scorey, scorex, (4+j)*scorey,0xBFBFBFFF);
						boxColor(screen, 1,(3+j)*scorey+1, scorex, (4+j)*scorey-1,0x2F2F2FFF);
						text = TTF_RenderText_Blended(fontbold, temptext, textColor);
					}
					else
					{
						boxColor(screen, 0,(3+j)*scorey, scorex, (4+j)*scorey,0xBFBFBFFF);
						boxColor(screen, 1,(3+j)*scorey+1, scorex-1, (4+j)*scorey-1,0x1F1F1FFF);
						text = TTF_RenderText_Blended(font, temptext, textColor);
					}
					textPosition.x = scorex/2-text->w/2;
					textPosition.y = 3.5*scorey+j*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
				}

				// Navigation
				right_button.x = 35*scorex;
				right_button.y =14*scorey+1;
				SDL_BlitSurface(right_image, 0, screen, &right_button);
				left_button.x = 20*scorex;
				left_button.y = 14*scorey+1;
				SDL_BlitSurface(left_image, 0, screen, &left_button);
				leftright_button.x = (21+songanzeigepattern)*scorex;
				leftright_button.y = 14*scorey;
				SDL_BlitSurface(leftright_image, 0, screen, &leftright_button);
			}

			// Arbeitsblatt Device
			else
			{
				if(isedit==false)
				{
					// All Notes Off
					allnotesoff_button.x = 24*scorex;
					allnotesoff_button.y = 22*scorey+2;
					SDL_BlitSurface(allnotesoff_image, 0, screen, &allnotesoff_button);

					// Timersource
					plug_button.x = 21*scorex;
					plug_button.y = 22*scorey+2;
					SDL_BlitSurface(plug_image, 0, screen, &plug_button);

					// Play, Pause, Stop
					if(timerrun==true)
					{
						play_button.x = 30*scorex;
						play_button.y = 22*scorey+1;
						SDL_BlitSurface(pause_image, 0, screen, &play_button);
					}
					else
					{
						play_button.x = 30*scorex;
						play_button.y = 22*scorey+1;
						SDL_BlitSurface(play_image, 0, screen, &play_button);
					}

					stop_button.x = 27*scorex;
					stop_button.y = 22*scorey+1;
					SDL_BlitSurface(stop_image, 0, screen, &stop_button);

					// forward, backward
					backward_button.x = 33*scorex;
					backward_button.y = 22*scorey+1;
					SDL_BlitSurface(backward_image, 0, screen, &backward_button);
					forward_button.x = 36*scorex;
					forward_button.y = 22*scorey+1;
					SDL_BlitSurface(forward_image, 0, screen, &forward_button);
				}


				// Exit Program
				shutdown_button.x = 2*scorex;
				shutdown_button.y = 22*scorey+1;
				SDL_BlitSurface(shutdown_image, 0, screen, &shutdown_button);

				// Load Song
				load_button.x = 5*scorex;
				load_button.y = 22*scorey+1;
				SDL_BlitSurface(load_image, 0, screen, &load_button);

				// Save Song
				save_button.x = 8*scorex;
				save_button.y = 22*scorey+2;
				SDL_BlitSurface(save_image, 0, screen, &save_button);

				// Edit Pattern
				edit_button.x = 11*scorex;
				edit_button.y = 22*scorey+1;
				if(isedit==true)
				{
					SDL_BlitSurface(speaker_image, 0, screen, &edit_button);
				}
				else
				{
					SDL_BlitSurface(edit_image, 0, screen, &edit_button);
				}

				if(isedit==true)
				{
					oktavedown_button.x = 27*scorex;
					oktavedown_button.y = 22*scorey+1;
					SDL_BlitSurface(oktavedown_image, 0, screen, &oktavedown_button);

					oktaveup_button.x = 30*scorex;
					oktaveup_button.y = 22*scorey+1;
					SDL_BlitSurface(oktaveup_image, 0, screen, &oktaveup_button);

					editcopy_button.x = 33*scorex;
					editcopy_button.y = 22*scorey+1;
					SDL_BlitSurface(editcopy_image, 0, screen, &editcopy_button);

					if(ispaste==true)
					{
						editpaste_button.x = 36*scorex;
						editpaste_button.y = 22*scorey+1;
						SDL_BlitSurface(editpaste_image, 0, screen, &editpaste_button);
					}

					// insert Notes
					if(selpatt[0]!=255)
					{
						if(strcmp(dsettings[seldev].type, "drum")==0)
						{
							if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][selpatt[3]][0]==0)
							{
								noteadd_button.x = 21*scorex;
								noteadd_button.y = 22*scorey+1;
								SDL_BlitSurface(noteadd_image, 0, screen, &noteadd_button);
							}

							if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][selpatt[3]][0]>0)
							{
								notedel_button.x = 21*scorex;
								notedel_button.y = 22*scorey+1;
								SDL_BlitSurface(notedel_image, 0, screen, &notedel_button);
							}
						}
						else if(strcmp(dsettings[seldev].type, "mono")==0)
						{
							if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]==0)
							{
								noteadd_button.x = 15*scorex;
								noteadd_button.y = 22*scorey+1;
								SDL_BlitSurface(noteadd_image, 0, screen, &noteadd_button);

								noteon_button.x = 18*scorex;
								noteon_button.y = 22*scorey+1;
								SDL_BlitSurface(noteon_image, 0, screen, &noteon_button);

								noteoff_button.x = 21*scorex;
								noteoff_button.y = 22*scorey+1;
								SDL_BlitSurface(noteoff_image, 0, screen, &noteoff_button);

								if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16-1][0][0]==2 or pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16-1][0][0]==3)
								{
									noteexpand_button.x = 24*scorex;
									noteexpand_button.y = 22*scorey+1;
									SDL_BlitSurface(noteexpand_image, 0, screen, &noteexpand_button);
								}
							}

							if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]>0)
							{
								notedel_button.x = 15*scorex;
								notedel_button.y = 22*scorey+1;
								SDL_BlitSurface(notedel_image, 0, screen, &notedel_button);
							}
						}
						else if(strcmp(dsettings[seldev].type, "poly")==0)
						{
							bool noteexists=false;
							bool noteonexists=false;
							for(int i=0;i<8;i++)
							{
								if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]>0)
								{
									if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]==11-selpatt[3]+(oktave[seldev]+2)*12)
									{
										noteexists=true;
									}
								}
								if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16-1][i][0]>0)
								{
									if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16-1][i][1]==11-selpatt[3]+(oktave[seldev]+2)*12)
									{
										noteonexists=true;
									}
								}
							}
							if(noteexists==false)
							{
								noteadd_button.x = 15*scorex;
								noteadd_button.y = 22*scorey+1;
								SDL_BlitSurface(noteadd_image, 0, screen, &noteadd_button);

								noteon_button.x = 18*scorex;
								noteon_button.y = 22*scorey+1;
								SDL_BlitSurface(noteon_image, 0, screen, &noteon_button);

								noteoff_button.x = 21*scorex;
								noteoff_button.y = 22*scorey+1;
								SDL_BlitSurface(noteoff_image, 0, screen, &noteoff_button);

								if(noteonexists==true)
								{
									noteexpand_button.x = 24*scorex;
									noteexpand_button.y = 22*scorey+1;
									SDL_BlitSurface(noteexpand_image, 0, screen, &noteexpand_button);
								}
							}
							else
							{
								notedel_button.x = 15*scorex;
								notedel_button.y = 22*scorey+1;
								SDL_BlitSurface(notedel_image, 0, screen, &notedel_button);
							}
						}
					}
				}


				SDL_FreeSurface(text);
				sprintf(temptext, "%s",dsettings[seldev].name);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 2*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				if(clockmodeext==true)
				{
					sprintf(temptext, "BPM: %s","ext");
				}
				else
				{
					sprintf(temptext, "BPM: %d",bpm);
				}
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 12*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				bpmrect = textPosition;

				SDL_FreeSurface(text);
				sprintf(temptext, "Time Division: 1/%d",timedivision);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 16*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				timedevrect = textPosition;

				SDL_FreeSurface(text);
				sprintf(temptext, "Steps: %d",maxstep+1);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 23*scorex;
				textPosition.y = 1.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				maxsteprect = textPosition;

				SDL_FreeSurface(text);
				sprintf(temptext, "Volume: %d",volume[seldev]);
				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
				textPosition.x = 34*scorex;
				textPosition.y = 2.5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				volumerect = textPosition;

				if(dsettings[seldev].maxprog>0)
				{
					SDL_FreeSurface(text);
					if(strcmp(dsettings[seldev].progtype,"GM")==0)
					{
						sprintf(temptext, "Program: %s", gm_program_name[selprog[seldev]]);
					}
					else
					{
						sprintf(temptext, "Program: %d",selprog[seldev]+1);
					}
					text = TTF_RenderText_Blended(fontbold, temptext, textColor);
					textPosition.x = 28*scorex;
					textPosition.y = 1.5*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					progrect = textPosition;
				}

				if(dsettings[seldev].maxbank>0)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "Bank: %d",selbank[seldev]);
					text = TTF_RenderText_Blended(fontbold, temptext, textColor);
					textPosition.x = 28*scorex;
					textPosition.y = 2.5*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					bankrect = textPosition;
				}

				// Debug

				
				if(selpatt[0]!=255 and selpatt[selpat[seldev]]!=255 and debug==true)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "[%d][%d]", mousescorex, mousescorey);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 20*scorex;
					textPosition.y = 2.5*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "selpatt: %d %d %d %d", selpatt[0], selpatt[1],selpatt[2]+startanzeigepattern*16,selpatt[3]);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 10*scorex;
					textPosition.y = 2.5*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "[%d][%d][%d]", pattern[selpatt[0]][selpatt[1]][selpatt[2]+startanzeigepattern*16][selpatt[3]][0], pattern[selpatt[0]][selpatt[1]][selpatt[2]+startanzeigepattern*16][selpatt[3]][1], pattern[selpatt[0]][selpatt[1]][selpatt[2]+startanzeigepattern*16][selpatt[3]][2]);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 16*scorex;
					textPosition.y = 2.5*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					sprintf(temptext, "[%d][%d][%d]", temppattern[selpatt[2]+startanzeigepattern*16][selpatt[3]][0], temppattern[selpatt[2]+startanzeigepattern*16][selpatt[3]][1], temppattern[selpatt[2]+startanzeigepattern*16][selpatt[3]][2]);
					text = TTF_RenderText_Blended(font, temptext, textColor);
					textPosition.x = 24*scorex;
					textPosition.y = 2.5*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);
				}

//				SDL_FreeSurface(text);
//				sprintf(temptext, "%s","Volume");
//				text = TTF_RenderText_Blended(fontbold, temptext, textColor);
//				textPosition.x = 2*scorex;
//				textPosition.y = 19.5*scorey-text->h/2;
//				SDL_BlitSurface(text, 0, screen, &textPosition);

				if(strcmp(dsettings[seldev].type,"drum")!=0)
				{
					if(dsettings[seldev].maxprog>0)
					{
						SDL_FreeSurface(text);
						sprintf(temptext, "%s","Program");
						text = TTF_RenderText_Blended(fontbold, temptext, textColor);
						textPosition.x = 2*scorex;
						textPosition.y = 20.5*scorey-text->h/2;
						SDL_BlitSurface(text, 0, screen, &textPosition);
					}
				}

				if(strcmp(dsettings[seldev].type,"drum")==0)
				{
					for(int i=0;i<16;i++)
					{
						SDL_FreeSurface(text);
						sprintf(temptext, "%s",drumsettings[i].name);
						text = TTF_RenderText_Blended(fontbold, temptext, textColor);
						textPosition.x = 2*scorex;
						textPosition.y = (i+5.5)*scorey-text->h/2;
						SDL_BlitSurface(text, 0, screen, &textPosition);
					}
					// Navigation
					right_button.x = 38*scorex;
					right_button.y = 21*scorey;
					SDL_BlitSurface(right_image, 0, screen, &right_button);
					left_button.x = 34*scorex;
					left_button.y = 21*scorey;
					SDL_BlitSurface(left_image, 0, screen, &left_button);
					leftright_button.x = (35+startanzeigepattern)*scorex;
					leftright_button.y = 21*scorey;
					SDL_BlitSurface(leftright_image, 0, screen, &leftright_button);
				}
				else if(strcmp(dsettings[seldev].type,"mono")==0 or strcmp(dsettings[seldev].type,"poly")==0)
				{

					for(int i=0;i<12;i++)
					{
						if(selkey==11-i)
						{
							boxColor(screen,4*scorex,(5+i)*scorey, 7*scorex, (6+i)*scorey,0xCFCFCFFF);
						}
						else
						{
							boxColor(screen,4*scorex,(5+i)*scorey, 7*scorex, (6+i)*scorey,0xFFFFFFFF);
						}
						klavier[i].x = 4*scorex;
						klavier[i].y = (5+i)*scorey;
						klavier[i].w = 3*scorex;
						klavier[i].h = (6+i)*scorey-(5+i)*scorey;
						if(i==1 or i==3 or i==5 or i==8 or i==10)
						{
							if(selkey==11-i)
							{
								boxColor(screen,4*scorex,(5+i)*scorey, 6*scorex, (6+i)*scorey,0x4F4F4FFF);
							}
							else
							{
								boxColor(screen,4*scorex,(5+i)*scorey, 6*scorex, (6+i)*scorey,0x1F1F1FFF);
							}
							klavier[i].x = 4*scorex;
							klavier[i].y = (5+i)*scorey;
							klavier[i].w = 2*scorex;
							klavier[i].h = (6+i)*scorey-(5+i)*scorey;
						}
					}
					lineColor(screen, 6*scorex, 6.7*scorey, 7*scorex, 6.7*scorey,0x1F1F1FFF);
					lineColor(screen, 6*scorex, 8.5*scorey, 7*scorex, 8.5*scorey,0x1F1F1FFF);
					lineColor(screen, 6*scorex, 10.3*scorey, 7*scorex, 10.3*scorey,0x1F1F1FFF);
					lineColor(screen, 4*scorex, 12*scorey, 7*scorex, 12*scorey,0x1F1F1FFF);
					lineColor(screen, 6*scorex, 13.6*scorey, 7*scorex, 13.6*scorey,0x1F1F1FFF);
					lineColor(screen, 6*scorex, 15.4*scorey, 7*scorex, 15.4*scorey,0x1F1F1FFF);

					for(int i=0;i<12;i++)
					{
						SDL_FreeSurface(text);
						sprintf(temptext, "%s%d",note[i],oktave[seldev]);
						text = TTF_RenderText_Blended(fontbold, temptext, textColor);
						textPosition.x = 2*scorex;
						textPosition.y = (i+5.5)*scorey-text->h/2;
						SDL_BlitSurface(text, 0, screen, &textPosition);
					}

					// Navigation
					up_button.x = 39*scorex;
					up_button.y = 5*scorey;
					SDL_BlitSurface(up_image, 0, screen, &up_button);
					down_button.x = 39*scorex;
					down_button.y = 16*scorey;
					SDL_BlitSurface(down_image, 0, screen, &down_button);
					updown_button.x = 39*scorex;
					updown_button.y = (13-oktave[seldev])*scorey;
					SDL_BlitSurface(updown_image, 0, screen, &updown_button);
					right_button.x = 38*scorex;
					right_button.y = 21*scorey;
					SDL_BlitSurface(right_image, 0, screen, &right_button);
					left_button.x = 34*scorex;
					left_button.y = 21*scorey;
					SDL_BlitSurface(left_image, 0, screen, &left_button);
					leftright_button.x = (35+startanzeigepattern)*scorex;
					leftright_button.y = 21*scorey;
					SDL_BlitSurface(leftright_image, 0, screen, &leftright_button);
				}

				// Raster
				for(int i=0;i<32;i++)
				{
					lineColor(screen, (7+i)*scorex, 4.2*scorey, (7+i)*scorex, 21*scorey,0x4F4F4FFF);
				}
				for(int i=0;i<16;i++)
				{
					lineColor(screen, 7*scorex, (5+i)*scorey, 39*scorex, (5+i)*scorey,0x4F4F4FFF);
				}

				// Time Division Marker
				for(int i=0;i<32/timedivision;i++)
				{
					lineColor(screen, (7+timedivision*i)*scorex, 4.2*scorey, (7+timedivision*i)*scorex, 21*scorey,0xFFFFFFFF);
					lineColor(screen, ((7+timedivision*i)+timedivision/4)*scorex, 4.2*scorey, ((7+timedivision*i)+timedivision/4)*scorex, 21*scorey,0x888888FF);
					lineColor(screen, ((7+timedivision*i)+2*timedivision/4)*scorex, 4.2*scorey, ((7+timedivision*i)+2*timedivision/4)*scorex, 21*scorey,0x888888FF);
					lineColor(screen, ((7+timedivision*i)+3*timedivision/4)*scorex, 4.2*scorey, ((7+timedivision*i)+3*timedivision/4)*scorex, 21*scorey,0x888888FF);
				}
				if(timedivision==32)
				{
					if(startanzeigepattern==1)
					{
						lineColor(screen, (7+16)*scorex, 4.2*scorey, (7+16)*scorex, 21*scorey,0xFFFFFFFF);
						lineColor(screen, (7+32)*scorex, 4.2*scorey, (7+32)*scorex, 21*scorey,0xFFFFFFFF);
					}
					else
					{
						lineColor(screen, (7)*scorex, 4.2*scorey, (7)*scorex, 21*scorey,0xFFFFFFFF);
						lineColor(screen, (7+32)*scorex, 4.2*scorey, (7+32)*scorex, 21*scorey,0xFFFFFFFF);
					}
				}

				if(timerrun==true)
				{
					if(aktstep<32)
					{
						startanzeigepattern=0;
					}
					else
					{
						startanzeigepattern=2;
					}
				}

				for(int i=0;i<32;i++)
				{
					if(aktstep==i+startanzeigepattern*16)
					{
						boxColor(screen, (7+i)*scorex+1,4*scorey+1, (8+i)*scorex-1, 5*scorey-1,0x2FFF2F44);
					}
					SDL_FreeSurface(text);
					sprintf(temptext, "%d",i+1+startanzeigepattern*16);
					text = TTF_RenderText_Blended(fontsmall, temptext, textColor);
					textPosition.x = (7.5+i)*scorex-text->w/2;
					textPosition.y = 4.5*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					for(int j=0;j<16;j++)
					{
//						boxColor(screen, (7+i)*scorex,(5+j)*scorey, (8+i)*scorex, (6+j)*scorey,0x6F6F6FFF);
//						boxColor(screen, (7+i)*scorex+1,(5+j)*scorey+1, (8+i)*scorex-1, (6+j)*scorey-1,0x2F2F2FFF);

						// Drum
						if(strcmp(dsettings[seldev].type,"drum")==0 and pattern[0][selpat[seldev]][i+startanzeigepattern*16][j][0]==1)
						{
							boxColor(screen, (7+i)*scorex+1,(5+j)*scorey+(127-pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][j][2])*scorey/128+1, (7+i)*scorex+scorex/2, (6+j)*scorey-1,0x2FFF2FFF);
						}
						else
						{
							if(j<12) // Notes
							{
								// Mono
								if(seldev>0 and seldev<5)
								{
									if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][0]==2)
									{
										if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][1]==(oktave[seldev]+2)*12+j)
										{
											boxColor(screen, (7+i)*scorex+1,(16-j)*scorey+(127-pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][2])*scorey/128+1, (7+i)*scorex+scorex-1, (17-j)*scorey-1,0x2FFF2FFF);
										}
									}
									else if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][0]==3)
									{
										if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][1]==(oktave[seldev]+2)*12+j)
										{
											boxColor(screen, (7+i)*scorex,(16-j)*scorey+(127-pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][2])*scorey/128+1, (7+i)*scorex+scorex-1, (17-j)*scorey-1,0x2FFF2FFF);
										}
									}
									if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][0]==7)
									{
										if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][1]==(oktave[seldev]+2)*12+j)
										{
											boxColor(screen, (7+i)*scorex+1,(16-j)*scorey+(127-pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][2])*scorey/128+1, (7+i)*scorex+scorex-1, (17-j)*scorey-1,0xFFFF2FFF);
										}
									}
									if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][0]==8)
									{
										if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][0][1]==(oktave[seldev]+2)*12+j)
										{
											boxColor(screen, (7+i)*scorex+1,(16-j)*scorey, (7+i)*scorex+scorex-1, (17-j)*scorey-1,0xFF2FFFFF);
										}
									}
								}
								// Poly
								else if(seldev>4 and seldev<8)
								{
									for(int k=0;k<8;k++)
									{
										if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][0]==2)
										{
											if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][1]==(oktave[seldev]+2)*12+j)
											{
												boxColor(screen, (7+i)*scorex+1,(16-j)*scorey+(127-pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][2])*scorey/128+1, (7+i)*scorex+scorex-1, (17-j)*scorey-1,0x2FFF2FFF);
											}
										}
										else if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][0]==3)
										{
											if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][1]==(oktave[seldev]+2)*12+j)
											{
												boxColor(screen, (7+i)*scorex,(16-j)*scorey+(127-pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][2])*scorey/128+1, (7+i)*scorex+scorex-1, (17-j)*scorey-1,0x2FFF2FFF);
											}
										}
										if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][0]==7)
										{
											if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][1]==(oktave[seldev]+2)*12+j)
											{
												boxColor(screen, (7+i)*scorex+1,(16-j)*scorey+(127-pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][2])*scorey/128+1, (7+i)*scorex+scorex-1, (17-j)*scorey-1,0xFFFF2FFF);
											}
										}
										if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][0]==8)
										{
											if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][k][1]==(oktave[seldev]+2)*12+j)
											{
												boxColor(screen, (7+i)*scorex+1,(16-j)*scorey, (7+i)*scorex+scorex-1, (17-j)*scorey-1,0xFF2FFFFF);
											}
										}
									}
								}
							}
	//						if(j==14) // Volume
	//						{
	//							if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][14][0]==6)
	//							{
	//								boxColor(screen, (7+i)*scorex+1,(5+j)*scorey+1, (8+i)*scorex-1, (6+j)*scorey-1,0x2F2FFFFF);
	//								SDL_FreeSurface(text);
	//								sprintf(temptext, "%d",pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][14][1]);
	//								text = TTF_RenderText_Blended(fontsmall, temptext, textColor);
	//								textPosition.x = (7.5+i)*scorex-text->w/2;
	//								textPosition.y = (5.5+j)*scorey-text->h/2;
	//								SDL_BlitSurface(text, 0, screen, &textPosition);
	//							}
	//						}
							if(j==15) // Program
							{
								if(pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][15][0]==6)
								{
									boxColor(screen, (7+i)*scorex+1,(5+j)*scorey+1, (8+i)*scorex-1, (6+j)*scorey-1,0x2F2FFFFF);
									SDL_FreeSurface(text);
									sprintf(temptext, "%d",pattern[seldev][selpat[seldev]][i+startanzeigepattern*16][15][1]+1);
									text = TTF_RenderText_Blended(fontsmall, temptext, textColor);
									textPosition.x = (7.5+i)*scorex-text->w/2;
									textPosition.y = (5.5+j)*scorey-text->h/2;
									SDL_BlitSurface(text, 0, screen, &textPosition);
								}
							}
						}
						// Selected
						if(selpatt[0]==seldev and selpatt[1]==selpat[seldev] and selpatt[2]==i and selpatt[3]==j)
						{
							boxColor(screen, (7+i)*scorex+1,(5+j)*scorey+1, (8+i)*scorex-1, (6+j)*scorey-1,0xBF2F2F88);
						}
					}
					if(i+startanzeigepattern*16>maxstep)
					{
						boxColor(screen, (7+i)*scorex+1,4*scorey, (8+i)*scorex, 21*scorey,0x00000088);
					}
				}
				lineColor(screen, 7*scorex, 5*scorey, 39*scorex, 5*scorey,0xFFFFFFFF);
				lineColor(screen, 7*scorex, 21*scorey, 39*scorex, 21*scorey,0xFFFFFFFF);
				if(seldev!=0)
				{
					lineColor(screen, 7*scorex, 17*scorey, 39*scorex, 17*scorey,0xFFFFFFFF);
				}
				// Patternleiste
				for(int j=0;j<16;j++)
				{
					SDL_FreeSurface(text);
					sprintf(temptext, "%d",j+1);
					if(selpat[seldev]==j)
					{
						boxColor(screen, 0,(3+j)*scorey, scorex, (4+j)*scorey,0xBFBFBFFF);
						boxColor(screen, 1,(3+j)*scorey+1, scorex, (4+j)*scorey-1,0x2F2F2FFF);
						text = TTF_RenderText_Blended(fontbold, temptext, textColor);
					}
					else
					{
						boxColor(screen, 0,(3+j)*scorey, scorex, (4+j)*scorey,0xBFBFBFFF);
						boxColor(screen, 1,(3+j)*scorey+1, scorex-1, (4+j)*scorey-1,0x1F1F1FFF);
						text = TTF_RenderText_Blended(font, temptext, textColor);
					}
					textPosition.x = scorex/2-text->w/2;
					textPosition.y = 3.5*scorey+j*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
				}

			}



			// Raster
			if(debug==true)
			{
				for(int i=0;i<40;i++)
				{
					for(int j=0;j<24;j++)
					{
						boxColor(screen, i*scorex,j*scorey,i*scorex,j*scorey,0xFFFFFFFF);
					}
				}
			}

			SDL_Flip(screen);
			anzeige=false;
			midiindata=false;
			midiinclockdata=false;
		}


		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			// Was ist passiert?
			switch(event.type)
			{
				case SDL_QUIT:
					// Das Programm soll beendet werden.
					// Wir merken uns das und brechen beim nächsten Mal die Schleife ab.
					run = false;
					break;
				case SDL_VIDEORESIZE:
					//Resize the screen
					screen = SDL_SetVideoMode( event.resize.w, event.resize.h, 32, SDL_DOUBLEBUF | SDL_RESIZABLE );

					//If there's an error
/*					if(!screen)
					{
					    std::cerr << "Konnte SDL-Fenster nicht erzeugen! Fehler: " << SDL_GetError() << std::endl;
					    return -1;
					}
*/
					scorex = screen->w/40;
					scorey = screen->h/24;

					fontbold = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", int(0.6*scorey));
					if(!fontbold)
					{
					    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
					    return -1;
					}
					font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", int(0.6*scorey));
					if(!font)
					{
					    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
					    return -1;
					}
					fontsmall = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", int(0.5*scorey));
					if(!fontsmall)
					{
					    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
					    return -1;
					}

					up_image = zoomSurface(up_image_org,float(scorex)/48,float(scorey)/48,1);
					down_image = zoomSurface(down_image_org,float(scorex)/48,float(scorey)/48,1);
					updown_image = zoomSurface(updown_image_org,float(scorex)/48,float(scorey)/48,1);
					right_image = zoomSurface(right_image_org,float(scorex)/48,float(scorey)/48,1);
					left_image = zoomSurface(left_image_org,float(scorex)/48,float(scorey)/48,1);
					leftright_image = zoomSurface(leftright_image_org,float(scorex)/48,float(scorey)/48,1);
					play_image = zoomSurface(play_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					pause_image = zoomSurface(pause_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					stop_image = zoomSurface(stop_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					plug_image = zoomSurface(plug_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					backward_image = zoomSurface(backward_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					forward_image = zoomSurface(forward_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					allnotesoff_image = zoomSurface(allnotesoff_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					save_image = zoomSurface(save_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					edit_image = zoomSurface(edit_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					load_image = zoomSurface(load_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					shutdown_image = zoomSurface(shutdown_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					speaker_image = zoomSurface(speaker_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					noteadd_image = zoomSurface(noteadd_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					noteexpand_image = zoomSurface(noteexpand_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					notedel_image = zoomSurface(notedel_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					noteon_image = zoomSurface(noteon_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					noteoff_image = zoomSurface(noteoff_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					oktavedown_image = zoomSurface(oktavedown_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					oktaveup_image = zoomSurface(oktaveup_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					editcopy_image = zoomSurface(editcopy_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					editcut_image = zoomSurface(editcut_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					editpaste_image = zoomSurface(editpaste_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					plus_image = zoomSurface(plus_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					minus_image = zoomSurface(minus_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					ende_image = zoomSurface(ende_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					repeat_image = zoomSurface(repeat_image_org,float(2*scorex-2)/48,float(2*scorey-2)/48,1);
					muted_image = zoomSurface(muted_image_org,float(2*scorex-2)/96,float(2*scorey-2)/96,1);
					notmuted_image = zoomSurface(notmuted_image_org,float(2*scorex-2)/96,float(2*scorey-2)/96,1);

					break;

				case SDL_KEYDOWN:
					keyPressed[event.key.keysym.sym] = true;
					if(keyPressed[SDLK_ESCAPE])
					{
						run = false;        // Programm beenden.
					}

					if(seldev==12)
					{
						string str;
						str = string(songname);

						if( str.length() <= 78 )
						{
							 if(event.key.keysym.sym == SDLK_BACKSPACE and str.length() > 0 )
							 {
							 //Remove a character from the end
								 str.erase( str.length()-1);
							 }
							 //If the key is a space
							 else if( event.key.keysym.unicode == (Uint16)' ')
							{
								 //Append the character
								str += (char)event.key.keysym.unicode;
							}
							 //If the key is a number
							 else if( ( event.key.keysym.unicode >= (Uint16)'0' ) && ( event.key.keysym.unicode <= (Uint16)'9' ) )
							 {
								 //Append the character
								 str += (char)event.key.keysym.unicode;
							 }
							 //If the key is a uppercase letter
							 else if( ( event.key.keysym.unicode >= (Uint16)'A' ) && ( event.key.keysym.unicode <= (Uint16)'Z' ) )
							 {
								 //Append the character
								 str += (char)event.key.keysym.unicode;
							 }
							 //If the key is a lowercase letter
							 else if( ( event.key.keysym.unicode >= (Uint16)'a' ) && ( event.key.keysym.unicode <= (Uint16)'z' ) )
							 {
								 //Append the character
								 str += (char)event.key.keysym.unicode;
							 }
							 //If backspace was pressed and the string isn't blank
						 }

						sprintf(songname,"%s",str.c_str());
					}
					if(seldev==14)
					{
						string str;
						str = string(texteingabe);

						if( str.length() <= 78 )
						{
							 if(event.key.keysym.sym == SDLK_BACKSPACE and str.length() > 0 )
							 {
							 //Remove a character from the end
								 str.erase( str.length()-1);
							 }
							 //If the key is a space
							 else if( event.key.keysym.unicode == (Uint16)' ')
							{
								 //Append the character
								str += (char)event.key.keysym.unicode;
							}
							 //If the key is a number
							 else if( ( event.key.keysym.unicode >= (Uint16)'0' ) && ( event.key.keysym.unicode <= (Uint16)'9' ) )
							 {
								 //Append the character
								 str += (char)event.key.keysym.unicode;
							 }
							 //If the key is a uppercase letter
							 else if( ( event.key.keysym.unicode >= (Uint16)'A' ) && ( event.key.keysym.unicode <= (Uint16)'Z' ) )
							 {
								 //Append the character
								 str += (char)event.key.keysym.unicode;
							 }
							 //If the key is a lowercase letter
							 else if( ( event.key.keysym.unicode >= (Uint16)'a' ) && ( event.key.keysym.unicode <= (Uint16)'z' ) )
							 {
								 //Append the character
								 str += (char)event.key.keysym.unicode;
							 }
							 //If backspace was pressed and the string isn't blank
						 }

						sprintf(texteingabe,"%s",str.c_str());
					}
					break;


				case SDL_KEYUP:
					keyPressed[event.key.keysym.sym] = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
			        if( event.button.button == SDL_BUTTON_LEFT )
			        {
			        	stillleftclick = true;

			        	// Check, if dclick
			        	leftclick = chrono::high_resolution_clock::now();
			        	std::chrono::duration<double> elapsed = leftclick - lastleftclick;
			        	if(elapsed.count()<0.2)
			        	{
			        		isleftdclick=true;
			        	}
			        	else
			        	{
			        		isleftdclick=false;
			        	}
			        	lastleftclick = chrono::high_resolution_clock::now();

			        	// Load Song
			        	if(seldev==11)
			        	{
							if(CheckMouse(mousex, mousey, ok_button)==true)
							{
								if(selloadsong<255)
								{
									sprintf(songpath, "%s/.sequencegang/songs/", getenv("HOME"));
									sprintf(songname,"%s",string(songdir[selloadsong+startdir]).c_str());
									ifstream infile(songpath+string(songdir[selloadsong+startdir])+".song", std::ios_base::binary);
									infile.read((char *)&pattern,sizeof(pattern));
									infile.close();
								}
								seldev=8;
								maxstep=int(pattern[8][0][0][10][1]);
								timedivision=int(pattern[8][0][0][9][1]);
							}
							if(CheckMouse(mousex, mousey, cancel_button)==true)
							{
								seldev=8;
							}
							if(CheckMouse(mousex, mousey, load_window)==true)
							{
								selloadsong = (mousey-4*scorey)/scorey;
							}
							if(CheckMouse(mousex, mousey, down_button)==true)
							{
								if(startdir<anzdir-maxdir)
								{
									startdir++;
									if(selloadsong>0)
									{
										selloadsong--;
									}
								}
							}
							if(CheckMouse(mousex, mousey, up_button)==true)
							{
								if(startdir>0)
								{
									startdir--;
									if(selloadsong<maxdir-1)
									{
										selloadsong++;
									}
								}
							}
			        	}
			        	// Save Song
			        	if(seldev==12)
			        	{
							if(CheckMouse(mousex, mousey, ok_button)==true)
							{
								sprintf(songpath, "%s/.sequencegang/songs/", getenv("HOME"));
								pattern[8][0][0][10][1]=maxstep;
								pattern[8][0][0][9][1]=timedivision;
								if(selloadsong<255 and strcmp(songname,"")==0)
								{
									ofstream outfile(songpath+string(songname)+".song", std::ios_base::binary);
									outfile.write((char *)&pattern,sizeof(pattern));
									outfile.close();
								}
								seldev=8;
							}
							if(CheckMouse(mousex, mousey, cancel_button)==true)
							{
								seldev=8;
							}
			        	}
			        	// Text Input
			        	if(seldev==14)
			        	{
							if(CheckMouse(mousex, mousey, ok_button)==true)
							{
								if(textinputmidiout==true)
								{
									sprintf(dsettings[seldevname].name,"%s",texteingabe);
								}
								if(textinputmidiin==true)
								{
									sprintf(midiindevname,"%s",texteingabe);
								}
								seldev=9;
							}
							if(CheckMouse(mousex, mousey, cancel_button)==true)
							{
								seldev=9;
							}
			        	}
			        	// Exit
			        	if(seldev==13)
			        	{
							if(CheckMouse(mousex, mousey, ok_button)==true)
							{
//								if(raspi==true)
//								{
//									system("shutdown -P now");
//								}
//								else
//								{
				        			run=false;
//				        		}
				        		break;
							}
							if(CheckMouse(mousex, mousey, cancel_button)==true)
							{
								seldev=8;
							}
			        	}

			        	if(seldev==9)
			        	{
							for(int i=0;i<8;i++)
							{
								if (CheckMouse(mousex, mousey, settings_devname_rect[i])==true)
								{
									seldevname=i;
									sprintf(texteingabe,"%s",dsettings[seldevname].name);
									sprintf(textinputtitel,"Device %d",i);
									textinputmidiout=true;
									textinputmidiin=false;
									seldev=14;
								}
							}

							if (CheckMouse(mousex, mousey, settings_midiindevname_rect)==true)
							{
								sprintf(texteingabe,"%s",midiindevname);
								sprintf(textinputtitel,"%s","MidiInDevice");
								textinputmidiin=true;
								textinputmidiout=false;
								seldev=14;
							}


				        	// Save Config
							if(CheckMouse(mousex, mousey, save_button)==true)
							{
								
								sprintf(dbpath, "%s/.sequencegang/settings.db", getenv("HOME"));
								if(sqlite3_open(dbpath, &settingsdb) != SQLITE_OK)
								{
									cout << "Fehler beim Öffnen: " << sqlite3_errmsg(settingsdb) << endl;
									return 1;
								}
								cout << "Datenbank erfolgreich geöffnet!" << endl;
							
								//Device Settings
								int i = 1;
								for(devicesettings dsettemp: dsettings)
								{
									
									sprintf(sql, "UPDATE settings SET name = \"%s\"  WHERE id = %d",dsettemp.name,i);
									if( sqlite3_exec(settingsdb,sql,writesettingscallback,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
										return 1;
									}
									sprintf(sql, "UPDATE settings SET mididevice = \"%d\" WHERE id = %d",dsettemp.mididevice,i);
									if( sqlite3_exec(settingsdb,sql,writesettingscallback,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
										return 1;
									}
									sprintf(sql, "UPDATE settings SET midichannel = \"%d\" WHERE id = %d",dsettemp.midichannel,i);
									if( sqlite3_exec(settingsdb,sql,writesettingscallback,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
										return 1;
									}
									sprintf(sql, "UPDATE settings SET maxbank = \"%d\" WHERE id = %d",dsettemp.maxbank,i);
									if( sqlite3_exec(settingsdb,sql,writesettingscallback,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
										return 1;
									}
									sprintf(sql, "UPDATE settings SET maxprog = \"%d\" WHERE id = %d",dsettemp.maxprog,i);
									if( sqlite3_exec(settingsdb,sql,writesettingscallback,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
										return 1;
									}
									sprintf(sql, "UPDATE settings SET type = \"%s\" WHERE id = %d",dsettemp.type,i);
									if( sqlite3_exec(settingsdb,sql,writesettingscallback,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
										return 1;
									}
									sprintf(sql, "UPDATE settings SET progtype = \"%s\" WHERE id = %d",dsettemp.progtype,i);
									if( sqlite3_exec(settingsdb,sql,writesettingscallback,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
										return 1;
									}

									i++;
								}
								
								// MidiInSettings
								sprintf(sql, "UPDATE midiinsettings SET name = \"%s\",mididevice = \"%d\",midichannel = \"%d\" WHERE id = 1",midiindevname,midiindevice,midiinch);
								if( sqlite3_exec(settingsdb,sql,writesettingscallback,0,0) != SQLITE_OK)
								{
									cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
									return 1;
								}
								sprintf(sql, "UPDATE midiinsettings SET name = \"%s\",mididevice = \"%d\" WHERE id = 2",midiinclockdevname,midiinclockdevice);
								if( sqlite3_exec(settingsdb,sql,writesettingscallback,0,0) != SQLITE_OK)
								{
									cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
									return 1;
								}

								sqlite3_close(settingsdb);

								
/*							
								//DrumSettings
								sprintf(sql, "SELECT * FROM drumsettings");
								if( sqlite3_exec(settingsdb,sql,drumsettingscallback,0,0) != SQLITE_OK)
								{
									cout << "Fehler beim SELECT: " << sqlite3_errmsg(settingsdb) << endl;
									return 1;
								}
							
								//MidiInSettings
								sprintf(sql, "SELECT * FROM midiinsettings");
								if( sqlite3_exec(settingsdb,sql,midiinsettingscallback,0,0) != SQLITE_OK)
								{
									cout << "Fehler beim SELECT: " << sqlite3_errmsg(settingsdb) << endl;
									return 1;
								}
							
								sqlite3_close(settingsdb);
*/

								
								
/*								file.open("Sequencegang.cfg",  ios::out);
								for(devicesettings dsettemp: dsettings)
								{
									file << dsettemp.name << endl;
									file << dsettemp.mididevice << endl;
									file << dsettemp.midichannel << endl;
									file << dsettemp.maxbank << endl;
									file << dsettemp.maxprog << endl;
									file << dsettemp.type << endl;
									file << dsettemp.progtype << endl;
								}
								file.close();
								file.open("Drumsettings.cfg",  ios::out);
								for(auto drsettemp: drumsettings)
								{
									file << drsettemp.name << endl;
									file << drsettemp.note << endl;
								}
								file.close();
								file.open("Midiin.cfg",  ios::out);
								file << midiindevname << endl;
								file << midiindevice << endl;
								file << midiinch << endl;
								file << midiinclockdevname << endl;
								file << midiinclockdevice << endl;
								file.close();
*/

							}
						}
			        	if(seldev<=8)
			        	{
							if(CheckMouse(mousex, mousey, load_button)==true)
							{
								directory.clear();
								sprintf(songpath, "%s/.sequencegang/songs/", getenv("HOME"));
								dir = opendir(songpath);
								while ((ent = readdir(dir)) != NULL)
								{
									directory.push_back(ent->d_name);
								}
								closedir (dir);

								songdir.clear();
								for(auto direct: directory)
								{
									if (direct.find (".song") != string::npos)
									{
										songdir.push_back(direct.substr(0, direct.length()-5));
									}
								}

								selloadsong=255;
								seldev=11;
							}
							if(CheckMouse(mousex, mousey, save_button)==true)
							{
				        		seldev=12;
							}
							if(CheckMouse(mousex, mousey, shutdown_button)==true)
							{
								seldev=13;
							}
							if(CheckMouse(mousex, mousey, edit_button)==true)
							{
								if(isedit==true)
								{
									isedit=false;
									selpatt[0]=255;
									selpatt[1]=255;
									selpatt[2]=255;
									selpatt[3]=255;
								}
								else
								{
									isedit=true;
								}
							}
			        	}

						if(seldev==8)
						{
							if(isedit==true)
							{
								if(CheckMouse(mousex, mousey, plus_button)==true)
								{
									if(selpatt[3]<8)
									{
										if(pattern[8][songtab][selpatt[2]][selpatt[3]][0]==0)
										{
											pattern[8][songtab][selpatt[2]][selpatt[3]][0]=1;
										}
										else
										{
											if(pattern[8][songtab][selpatt[2]][selpatt[3]][1]<16)
											{
												pattern[8][songtab][selpatt[2]][selpatt[3]][1]++;
											}
										}
									}
									if(selpatt[3]==8)
									{
										if(pattern[8][songtab][selpatt[2]][8][0]==0)
										{
											pattern[8][songtab][selpatt[2]][8][0]=2;
											pattern[8][songtab][selpatt[2]][8][1]=bpm/4;
										}
										else
										{
											if(pattern[8][songtab][selpatt[2]][8][1]<255)
											{
												pattern[8][songtab][selpatt[2]][8][1]++;
											}
										}
									}
								}
								if(CheckMouse(mousex, mousey, minus_button)==true)
								{
									if(selpatt[3]<8)
									{
										if(pattern[8][songtab][selpatt[2]][selpatt[3]][1]>0)
										{
											pattern[8][songtab][selpatt[2]][selpatt[3]][1]--;
										}
										else
										{
											if(pattern[8][songtab][selpatt[2]][selpatt[3]][0]==1)
											{
												pattern[8][songtab][selpatt[2]][selpatt[3]][0]=0;
											}
										}
									}
									if(selpatt[3]==8)
									{
										if(pattern[8][songtab][selpatt[2]][8][1]>5)
										{
											pattern[8][songtab][selpatt[2]][8][1]--;
										}
										else
										{
											if(pattern[8][songtab][selpatt[2]][8][0]==2)
											{
												pattern[8][songtab][selpatt[2]][8][0]=0;
											}
										}
									}
								}
								if(CheckMouse(mousex, mousey, ende_button)==true)
								{
									if(selpatt[3]==8)
									{
										if(pattern[8][songtab][selpatt[2]][8][0]==0)
										{
											pattern[8][songtab][selpatt[2]][8][0]=3;
										}
										else
										{
											pattern[8][songtab][selpatt[2]][8][0]=0;
										}
									}
								}
								if(CheckMouse(mousex, mousey, repeat_button)==true)
								{
									if(selpatt[3]==8)
									{
										if(pattern[8][songtab][selpatt[2]][8][0]==0)
										{
											pattern[8][songtab][selpatt[2]][8][0]=4;
										}
										else
										{
											pattern[8][songtab][selpatt[2]][8][0]=0;
										}
									}
								}
							}
			        	}

			        	if(seldev<8)
			        	{
			        		if(isedit==false)
			        		{
								// All Sounds Off
								if(CheckMouse(mousex, mousey, allnotesoff_button)==true)
								{
									if(dsettings[seldev].mididevice==255)
									{
										fluid_synth_all_sounds_off(fluid_synth, dsettings[seldev].midichannel-1);
									}
									else if(dsettings[playdev].mididevice==254)
									{
										// Sampler
									}
									else
									{
										wsmidi.AllSoundsOff(dsettings[seldev].mididevice, dsettings[seldev].midichannel-1);
									}
								}
								// Timersource
								if(CheckMouse(mousex, mousey, plug_button)==true)
								{
									if(clockmodeext==true)
									{
										clockmodeext=false;
									}
									else
									{
										clockmodeext=true;
									}
								}
								// Play/Pause
								if(CheckMouse(mousex, mousey, play_button)==true)
								{
									if(timerrun==true)
									{
										timerrun=false;
									}
									else
									{
										timerrun=true;
										playsong=false;
									}
								}
								// Stop
								if(CheckMouse(mousex, mousey, stop_button)==true)
								{
									timerrun=false;
									aktstep=64;
									if(strcmp(dsettings[seldev].type, "mono")==0)
									{
										if(dsettings[seldev].mididevice==255)
										{
											fluid_synth_all_notes_off(fluid_synth, dsettings[seldev].midichannel-1);
										}
										else if(dsettings[seldev].mididevice==254)
										{
											// Sampler
										}
										else
										{
											wsmidi.AllNotesOff(dsettings[seldev].mididevice, dsettings[seldev].midichannel-1);
										}
									}
									if(strcmp(dsettings[seldev].type, "poly")==0)
									{
										if(dsettings[seldev].mididevice==255)
										{
											fluid_synth_all_notes_off(fluid_synth, dsettings[seldev].midichannel-1);
										}
										else if(dsettings[seldev].mididevice==254)
										{
											// Sampler
										}
										else
										{
											wsmidi.AllNotesOff(dsettings[seldev].mididevice, dsettings[seldev].midichannel-1);
										}
									}
								}
								// backward
								if(CheckMouse(mousex, mousey, backward_button)==true)
								{
									if(aktstep>0)
									{
										aktstep--;
									}
									else
									{
										aktstep=maxstep;
									}
								}
								// forward
								if(CheckMouse(mousex, mousey, forward_button)==true)
								{
									if(aktstep<maxstep)
									{
										aktstep++;
									}
									else
									{
										aktstep=0;
									}
								}
			        		}

							if(isedit==true)
							{
								// Copy
								if(CheckMouse(mousex, mousey, editcopy_button)==true)
								{
									for(int i=0;i<64;i++)
									{
										for(int j=0;j<16;j++)
										{
											temppattern[i][j][0]=pattern[seldev][selpat[seldev]][i][j][0];
											temppattern[i][j][1]=pattern[seldev][selpat[seldev]][i][j][1];
											temppattern[i][j][2]=pattern[seldev][selpat[seldev]][i][j][2];
										}
									}
									ispaste=true;
								}
								// Paste
								if(CheckMouse(mousex, mousey, editpaste_button)==true)
								{
									for(int i=0;i<64;i++)
									{
										for(int j=0;j<16;j++)
										{
											pattern[seldev][selpat[seldev]][i][j][0]=temppattern[i][j][0];
											pattern[seldev][selpat[seldev]][i][j][1]=temppattern[i][j][1];
											pattern[seldev][selpat[seldev]][i][j][2]=temppattern[i][j][2];
										}
									}
								}
								// Oktave down
								if(CheckMouse(mousex, mousey, oktavedown_button)==true)
								{
									for(int i=0;i<64;i++)
									{
										for(int j=0;j<16;j++)
										{
											if(pattern[seldev][selpat[seldev]][i][j][0]==2 or pattern[seldev][selpat[seldev]][i][j][0]==3)
											{
												if(pattern[seldev][selpat[seldev]][i][j][1]>12)
												{
													pattern[seldev][selpat[seldev]][i][j][1]=pattern[seldev][selpat[seldev]][i][j][1]-12;
												}
											}
										}
									}
								}
								// Oktave up
								if(CheckMouse(mousex, mousey, oktaveup_button)==true)
								{
									for(int i=0;i<64;i++)
									{
										for(int j=0;j<16;j++)
										{
											if(pattern[seldev][selpat[seldev]][i][j][0]==2 or pattern[seldev][selpat[seldev]][i][j][0]==3)
											{
												if(pattern[seldev][selpat[seldev]][i][j][1]<115)
												{
													pattern[seldev][selpat[seldev]][i][j][1]=pattern[seldev][selpat[seldev]][i][j][1]+12;
												}
											}
										}
									}
								}
								// Pattern
								if(mousescorex>6 and mousescorex<39 and mousescorey>4 and mousescorey<21)
								{
//									if(mousescorey-5<12 or mousescorey-5==15)
//									{
										selpatt[0]=seldev;
										selpatt[1]=selpat[seldev];
										selpatt[2]=mousescorex-7;
										selpatt[3]=mousescorey-5;
										if(seldev==0)
										{
											if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][0]==1)
											{
												volume[seldev]=pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2];
											}
										}
										else
										{
											if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]==1 or pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]==2)
											{
												volume[seldev]=pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2];
											}

										}
//									}
								}
//			        			cout << selpatt[0] << " " << selpatt[1] << " " << selpatt[2] << " " << selpatt[3] << endl;
//								cout << mousescorey-5 << " " << int(pattern[seldev][selpat[seldev]][selpatt[2]][selpatt[3]][1]) <<  " " << selpatt[3]+(oktave[seldev]+2)*12 << endl;
			        		}
			        	}
			        	if(seldev==8)
			        	{
			        		if(isedit==false)
			        		{
								// All Sounds Off
								if(CheckMouse(mousex, mousey, allnotesoff_button)==true)
								{
									for(int i=0;i<8;i++)
									{
										if(dsettings[i].mididevice==255)
										{
											fluid_synth_all_sounds_off(fluid_synth, dsettings[i].midichannel-1);
										}
										else if(dsettings[i].mididevice==254)
										{
											// Sampler
										}
										else
										{
											wsmidi.AllSoundsOff(dsettings[i].mididevice, dsettings[i].midichannel-1);
										}
									}
								}
								// Timersource
								if(CheckMouse(mousex, mousey, plug_button)==true)
								{
									if(clockmodeext==true)
									{
										clockmodeext=false;
									}
									else
									{
										clockmodeext=true;
									}
								}
								// Play/Pause
								if(CheckMouse(mousex, mousey, play_button)==true)
								{
									if(timerrun==true)
									{
										timerrun=false;
									}
									else
									{
										timerrun=true;
										playsong=true;
										if(aktstep==64)
										{
											songtab=8;
										}
									}
								}
								// Stop
								if(CheckMouse(mousex, mousey, stop_button)==true)
								{
									timerrun=false;
									aktstep=64;
									songstep=64;
									songtab=0;
									playsong=false;
									for(int i=0;i<8;i++)
									{
										if(dsettings[seldev].mididevice==255)
										{
											fluid_synth_all_notes_off(fluid_synth, dsettings[seldev].midichannel-1);
										}
										else if(dsettings[seldev].mididevice==254)
										{
											// Sampler
										}
										else
										{
											wsmidi.AllNotesOff(dsettings[seldev].mididevice, dsettings[seldev].midichannel-1);
										}
									}
								}
								// backward
								if(CheckMouse(mousex, mousey, backward_button)==true)
								{
									if(songstep>0)
									{
										songstep--;
										aktstep=0;
									}
									else
									{
										songstep=63;
										aktstep=0;
									}
								}
								// forward
								if(CheckMouse(mousex, mousey, forward_button)==true)
								{
									if(songstep<63)
									{
										songstep++;
										aktstep=0;
									}
									else
									{
										songstep=0;
										aktstep=0;
									}
								}
								for(int i=0;i<8;i++)
								{
									if(CheckMouse(mousex, mousey, mute_button[i])==true)
									{
										if(ismuted[i]==true)
										{
											ismuted[i]=false;
										}
										else
										{
											ismuted[i]=true;
										}
									}
								}
			        		}

							// Scrollbalken Pattern
							if(CheckMouse(mousex, mousey, leftright_button)==true)
							{
								leftright_aktiv = true;
							}
							if(CheckMouse(mousex, mousey, left_button)==true)
							{
								if(songanzeigepattern>0)
								{
									songanzeigepattern--;
								}
							}
							if(CheckMouse(mousex, mousey, right_button)==true)
							{
								if(songanzeigepattern<13)
								{
									songanzeigepattern++;
								}
							}

							// Pattern
							if(isedit==true)
							{
								if(mousescorex>11 and mousescorex<36 and mousescorey>4 and mousescorey<14)
								{
									if(mousescorey-5<9)
									{
										selpatt[0]=8;
										selpatt[1]=songtab;
										selpatt[2]=(mousescorex-12)/2+songanzeigepattern*4;
										selpatt[3]=mousescorey-5;
									}
	//			        			cout << selpatt[0] << " " << selpatt[1] << " " << selpatt[2] << " " << selpatt[3] << endl;
	//								cout << mousescorey-5 << " " << int(pattern[seldev][selpat[seldev]][selpatt[2]][selpatt[3]][1]) <<  " " << selpatt[3]+(oktave[seldev]+2)*12 << endl;
								}
							}
				        	if(mousescorex==0)
				        	{
				        		if(mousescorey>2 and mousescorey<11)
				        		{
									songtab = mousescorey-3;
				        		}
				        	}
			        	}
			        	if(mousescorey==0)
			        	{
				        	selpatt[0] = 255;
							selpatt[1] = 255;
							selpatt[2] = 255;
							selpatt[3] = 255;

			        		if(mousescorex>3 and mousescorex < 20)
			        		{
								seldev=(mousescorex-4)/2;
				        		break;
			        		}
			        		else if(mousescorex>0 and mousescorex < 4)
			        		{
								seldev=8;
				        		break;
			        		}
			        		else if(mousescorex>24 and mousescorex < 29)
			        		{
								seldev=16;
								fluid_program_state.clear();
							    for(int i=0;i<16;i++)
							    {
									if (FLUID_OK == fluid_synth_get_program (fluid_synth, i, &sfid, &bank, &program))
									{
										fptmp.channel=i;
										fptmp.bank=bank;
										fptmp.program=program;
										fluid_program_state.push_back(fptmp);
									}
							    }
				        		break;
			        		}
			        		else if(mousescorex>20 and mousescorex < 25)
			        		{
								seldev=17;
				        		break;
			        		}
			        		else if(mousescorex>28 and mousescorex < 33)
			        		{
								seldev=15;
				        		break;
			        		}
			        		else if(mousescorex>32 and mousescorex < 37)
			        		{
								seldev=9;
				        		break;
			        		}
			        		else if(mousescorex>36)
			        		{
								seldev=10;
				        		break;
			        		}
			        	}
			        	if(mousescorex==0)
			        	{
				        	selpatt[0] = 255;
							selpatt[1] = 255;
							selpatt[2] = 255;
							selpatt[3] = 255;

			        		if(mousescorey>2 and mousescorey<19)
			        		{
								selpat[seldev] = mousescorey-3;
			        		}
			        	}
						if(seldev>0 and seldev<8)
						{
							// Scollbalken Oktave
							if(CheckMouse(mousex, mousey, updown_button)==true)
							{
								updown_aktiv = true;
							}
							if(CheckMouse(mousex, mousey, down_button)==true)
							{
								if(oktave[seldev]>-2)
								{
									oktave[seldev]--;
								}
							}
							if(CheckMouse(mousex, mousey, up_button)==true)
							{
								if(oktave[seldev]<7)
								{
									oktave[seldev]++;
								}
							}
						}
						if(seldev==17)
						{
							// Oktave
							if(CheckMouse(mousex, mousey, up_button)==true)
							{
								if(oktave[seldev]>-2)
								{
									oktave[seldev]--;
								}
							}
							if(CheckMouse(mousex, mousey, down_button)==true)
							{
								if(oktave[seldev]<5)
								{
									oktave[seldev]++;
								}
							}
							if(mousescorex>3 and mousescorex<20 and mousescorey>4 and mousescorey<21)
							{
								selkey = mousescorey-5;
								klavlastnote = selkey+(oktave[seldev]+2)*16;

								if(sndbnk[klavlastnote].channel==-1)
								{
									wsmidi.PlaySample(klavlastnote);
								}
							}
							for(int i=0;i<16;i++)
							{
								if(CheckMouse(mousex, mousey, opensample_button[i])==true)
								{
									if(strcmp(sndbnk[i+(oktave[seldev]+2)*16].name,"X")!=0)
									{
										cout << "Delete Sample" << i+(oktave[seldev]+2)*16 << endl;
										del_sample=i+(oktave[seldev]+2)*16;
										seldev=18;
									}
									else
									{
										cout << "Open Sample" << i+(oktave[seldev]+2)*16 << endl;

										directory.clear();
										sprintf(samplepath, "%s/.sequencegang/samples/", getenv("HOME"));
										dir = opendir(samplepath);
										while ((ent = readdir(dir)) != NULL)
										{
											directory.push_back(ent->d_name);
										}
										closedir (dir);

										sampledir.clear();
										for(auto direct: directory)
										{
											if (direct.find (".wav") != string::npos)
											{
												sampledir.push_back(direct);
											}
										}
										sel_sample=i+(oktave[seldev]+2)*16;
										startdir=0;
										anzdir=0;
										selloadsample=255;
										seldev=19;
									}
								}
							}
						}
						if(seldev==18)
						{
							if(CheckMouse(mousex, mousey, ok_button)==true)
							{
								sprintf(sndbnk[del_sample].name,"X");
								sndbnk[del_sample].sound = NULL;

								sprintf(dbpath, "%s/.sequencegang/samplebank.db", getenv("HOME"));
								if(sqlite3_open(dbpath, &soundbankdb) != SQLITE_OK)
								{
									cout << "Fehler beim Öffnen: " << sqlite3_errmsg(soundbankdb) << endl;
									return 1;
								}
								cout << "Sampledatenbank erfolgreich geöffnet!" << endl;

								sprintf(sql, "DELETE FROM samples WHERE key = %d;",del_sample);
								if( sqlite3_exec(soundbankdb,sql,0,0,0) != SQLITE_OK)
								{
									cout << "Fehler beim DELETE: " << sqlite3_errmsg(soundbankdb) << endl;
									return 1;
								}

								sqlite3_close(soundbankdb);
								del_sample=255;
								seldev=17;
							}
							if(CheckMouse(mousex, mousey, cancel_button)==true)
							{
								seldev=17;
							}
						}

			        	// Load Sample
			        	if(seldev==19)
			        	{
							if(CheckMouse(mousex, mousey, ok_button)==true)
							{
								if(selloadsample<255)
								{
									sprintf(samplename,"%s",string(sampledir[selloadsample+startdir]).c_str());
									char cstring[256];
									sprintf(sndbnk[sel_sample].name,"%s",samplename);
									sprintf(sndbnk[sel_sample].path,"%s%s","samples/",samplename);
									sprintf(cstring,"%s%s","samples/",samplename);
									sndbnk[sel_sample].sound=Mix_LoadWAV(cstring);


									sprintf(dbpath, "%s/.sequencegang/samplebank.db", getenv("HOME"));
									if(sqlite3_open(dbpath, &soundbankdb) != SQLITE_OK)
									{
										cout << "Fehler beim Öffnen: " << sqlite3_errmsg(soundbankdb) << endl;
										return 1;
									}
									cout << "Sampledatenbank erfolgreich geöffnet!" << endl;
									sprintf(sql, "INSERT INTO samples ('key','name','path') VALUES (%d,'%s','%s');",sel_sample,sndbnk[sel_sample].name,sndbnk[sel_sample].path);
									if( sqlite3_exec(soundbankdb,sql,0,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim INSERT: " << sqlite3_errmsg(soundbankdb) << endl;
										return 1;
									}
									sqlite3_close(soundbankdb);



								}
								del_sample=255;
								seldev=17;
							}
							if(CheckMouse(mousex, mousey, cancel_button)==true)
							{
								seldev=17;
							}
							if(CheckMouse(mousex, mousey, load_window)==true)
							{
								selloadsample = (mousey-4*scorey)/scorey;
							}
							if(CheckMouse(mousex, mousey, down_button)==true)
							{
								if(startdir<anzdir-maxdir)
								{
									startdir++;
									if(selloadsample>0)
									{
										selloadsample--;
									}
								}
							}
							if(CheckMouse(mousex, mousey, up_button)==true)
							{
								if(startdir>0)
								{
									startdir--;
									if(selloadsample<maxdir-1)
									{
										selloadsample++;
									}
								}
							}
			        	}
						if(seldev<8)
						{
				        	// Klaviatur Note On
							if(strcmp(dsettings[seldev].type, "drum")==0)
							{
								if(mousescorex>3 and mousescorex<7 and mousescorey>4 and mousescorey<23)
								{
									selkey = 16-mousescorey;
									klavlastnote = drumsettings[11-selkey].note;
									if(dsettings[seldev].mididevice==255)
									{
									    fluid_synth_noteon(fluid_synth, dsettings[seldev].midichannel-1, klavlastnote, 100);
									}
									else
									{
										wsmidi.NoteOn(dsettings[seldev].mididevice, dsettings[seldev].midichannel-1, klavlastnote, volume[seldev]);
									}
								}
							}
							else
							{			        	
								if(mousescorex>3 and mousescorex<7 and mousescorey>4 and mousescorey<17)
								{
									selkey = 16-mousescorey;
									klavlastnote = selkey+(oktave[seldev]+2)*12;
									if(dsettings[seldev].mididevice==255)
									{
									    fluid_synth_noteon(fluid_synth, dsettings[seldev].midichannel-1, klavlastnote, volume[seldev]);
									}
									else if(dsettings[seldev].mididevice==254)
									{
										wsmidi.PlaySample(klavlastnote);
									}
									else
									{
										wsmidi.NoteOn(dsettings[seldev].mididevice, dsettings[seldev].midichannel-1, klavlastnote, volume[seldev]);
									}
								}
							}
							// Scrollbalken Pattern
							if(CheckMouse(mousex, mousey, leftright_button)==true)
							{
								leftright_aktiv = true;
							}
							if(CheckMouse(mousex, mousey, left_button)==true)
							{
								if(startanzeigepattern>0)
								{
									startanzeigepattern--;
								}
							}
							if(CheckMouse(mousex, mousey, right_button)==true)
							{
								if(startanzeigepattern<2)
								{
									startanzeigepattern++;
								}
							}
						}
			        	// Note hinzufügen bei dclick
			        	if(isleftdclick==true and isedit==true)
			        	{
			        		if(mousescorex>6 and mousescorex<39 and mousescorey>4 and mousescorey<21)
			        		{
			        			if(selpatt[0]==8)
			        			{
			        				if(selpatt[3]<8)
			        				{
										if(pattern[selpatt[0]][selpatt[1]][selpatt[2]][selpatt[3]][0]==0)
										{
											pattern[selpatt[0]][selpatt[1]][selpatt[2]][selpatt[3]][0]=1;
										}
			        				}
			        			}
			        			else if(selpatt[0]==seldev and selpatt[1]==selpat[seldev] and selpatt[2]==mousescorex-7 and selpatt[3]==mousescorey-5)
			        			{
		        					if(strcmp(dsettings[seldev].type, "drum")==0)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][0]==0)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][0]=1;
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][1]=drumsettings[mousescorey-5].note;
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2]=volume[seldev];
										}
									}
			        				else if(selpatt[3]<12) // Noten
			        				{
										if(strcmp(dsettings[seldev].type, "mono")==0)
										{
											if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]==0)
											{
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]=2;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][1]=(16-mousescorey)+(oktave[seldev]+2)*12;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]=volume[seldev];
											}
										}
										else if(strcmp(dsettings[seldev].type, "poly")==0)
										{
											bool noteexists=false;
											int exists=255;
											for(int i=0;i<8;i++)
											{
												if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]>0)
												{
													if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]==11-selpatt[3]+(oktave[seldev]+2)*12)
													{
														noteexists=true;
													}
												}
											}
											if(noteexists==false)
											{
												for(int i=0;i<8;i++)
												{
													if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]==0)
													{
														pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]=2;
														pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]=(16-mousescorey)+(oktave[seldev]+2)*12;
														pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][2]=volume[seldev];
														break;
													}
												}
											}
										}
			        				}
			        				else if(selpatt[3]==15) // Program
			        				{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][0]==0)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][0]=6;
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][1]=selprog[seldev];
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2]=0;
										}
			        				}
			        			}
			        		}
			        	}
			        	if(isedit==true)
			        	{
							// insert/delete Note
							if(strcmp(dsettings[seldev].type, "drum")==0)
							{
								if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][selpatt[3]][0]==0)
								{
									if(CheckMouse(mousex, mousey, noteadd_button)==true)
									{
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][selpatt[3]][0]=1;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][selpatt[3]][1]=drumsettings[selpatt[3]].note;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][selpatt[3]][2]=volume[seldev];
									}
								}
								else
								{
									if(CheckMouse(mousex, mousey, notedel_button)==true)
									{
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][selpatt[3]][0]=0;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][selpatt[3]][1]=0;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][selpatt[3]][2]=0;
									}
								}
							}
							else if(strcmp(dsettings[seldev].type, "mono")==0)
							{
								if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]==0)
								{
									if(CheckMouse(mousex, mousey, noteadd_button)==true)
									{
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]=2;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][1]=11-selpatt[3]+(oktave[seldev]+2)*12;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]=volume[seldev];
										if(selpatt[2]<63)
										{
											selpatt[2]++;
										}
									}
									if(CheckMouse(mousex, mousey, noteon_button)==true)
									{
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]=7;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][1]=11-selpatt[3]+(oktave[seldev]+2)*12;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]=volume[seldev];
										if(selpatt[2]<63)
										{
											selpatt[2]++;
										}
									}
									if(CheckMouse(mousex, mousey, noteoff_button)==true)
									{
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]=8;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][1]=11-selpatt[3]+(oktave[seldev]+2)*12;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]=0;
										if(selpatt[2]<63)
										{
											selpatt[2]++;
										}
									}
									if(CheckMouse(mousex, mousey, noteexpand_button)==true)
									{
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]=3;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][1]=pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16-1][0][1];
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]=volume[seldev];
										if(selpatt[2]<63)
										{
											selpatt[2]++;
										}
									}
								}
								else
								{
									if(CheckMouse(mousex, mousey, notedel_button)==true)
									{
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]=0;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][1]=0;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]=0;
									}
								}
							}
							else if(strcmp(dsettings[seldev].type, "poly")==0)
							{
								if(CheckMouse(mousex, mousey, noteadd_button)==true)
								{
									bool noteexists=false;
									int exists=255;
									for(int i=0;i<8;i++)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]>0)
										{
											if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]==11-selpatt[3]+(oktave[seldev]+2)*12)
											{
												noteexists=true;
											}
										}
									}
									if(noteexists==false)
									{
										for(int i=0;i<8;i++)
										{
											if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]==0)
											{
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]=2;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]=11-selpatt[3]+(oktave[seldev]+2)*12;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][2]=volume[seldev];
												if(selpatt[2]<63)
												{
													selpatt[2]++;
												}
												break;
											}
										}
									}
								}
								if(CheckMouse(mousex, mousey, noteon_button)==true)
								{
									bool noteexists=false;
									int exists=255;
									for(int i=0;i<8;i++)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]>0)
										{
											if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]==11-selpatt[3]+(oktave[seldev]+2)*12)
											{
												noteexists=true;
											}
										}
									}
									if(noteexists==false)
									{
										for(int i=0;i<8;i++)
										{
											if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]==0)
											{
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]=7;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]=11-selpatt[3]+(oktave[seldev]+2)*12;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][2]=volume[seldev];
												if(selpatt[2]<63)
												{
													selpatt[2]++;
												}
												break;
											}
										}
									}
								}
								if(CheckMouse(mousex, mousey, noteoff_button)==true)
								{
									bool noteexists=false;
									int exists=255;
									for(int i=0;i<8;i++)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]>0)
										{
											if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]==11-selpatt[3]+(oktave[seldev]+2)*12)
											{
												noteexists=true;
											}
										}
									}
									if(noteexists==false)
									{
										for(int i=0;i<8;i++)
										{
											if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]==0)
											{
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]=8;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]=11-selpatt[3]+(oktave[seldev]+2)*12;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][2]=0;
												if(selpatt[2]<63)
												{
													selpatt[2]++;
												}
												break;
											}
										}
									}
								}
								if(CheckMouse(mousex, mousey, noteexpand_button)==true)
								{
									for(int i=0;i<8;i++)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]==0)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]=3;
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]=11-selpatt[3]+(oktave[seldev]+2)*12;
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][2]=volume[seldev];
											{
												selpatt[2]++;
											}
											break;
										}
									}
								}
								if(CheckMouse(mousex, mousey, notedel_button)==true)
								{
									for(int i=0;i<8;i++)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]==11-selpatt[3]+(oktave[seldev]+2)*12)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]=0;
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]=0;
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][2]=0;
										}
									}
								}
							}
			        	}
			        }
			        if( event.button.button == SDL_BUTTON_RIGHT )
			        {
			        	if(isedit==true)
			        	{
							if(selpatt[0]==8)
							{
								if(selpatt[3]<8)
								{
									if(selpatt[2]==(mousescorex-12)/2+songanzeigepattern*4)
										if(pattern[selpatt[0]][selpatt[1]][selpatt[2]][selpatt[3]][0]==1)
										{
											pattern[selpatt[0]][selpatt[1]][selpatt[2]][selpatt[3]][0]=0;
										}
								}
							}
							if(mousescorex>6 and mousescorex<39 and mousescorey>4 and mousescorey<21)
							{
								if(strcmp(dsettings[seldev].type, "drum")==0)
								{
									if(selpatt[3]<16) // Noten
									{
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][0]=0;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][1]=0;
										pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2]=0;
									}
								}
								else
								{
									if(selpatt[3]<12) // Noten
									{
										if(selpatt[0]==seldev and selpatt[1]==selpat[seldev] and selpatt[2]==mousescorex-7 and selpatt[3]==mousescorey-5)
										{
											if(strcmp(dsettings[seldev].type, "mono")==0)
											{
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]=0;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][1]=0;
												pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]=0;
											}
											else if(strcmp(dsettings[seldev].type, "poly")==0)
											{
												for(int i=0;i<8;i++)
												{
													if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]==11-selpatt[3]+(oktave[seldev]+2)*12)
													{
														pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][0]=0;
														pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][1]=0;
														pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][i][2]=0;
														break;
													}
												}
											}
										}
										if(selpatt[3]==15) // Program
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][0]=0;
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][1]=0;
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2]=0;
										}
									}
								}
							}
			        	}
			        }
			        if( event.button.button == SDL_BUTTON_WHEELUP )
			        {
						if(CheckMouse(mousex, mousey, volumerect)==true)
						{
							if(volume[seldev]<127)
							{
								volume[seldev]++;
							}
						}
						if(CheckMouse(mousex, mousey, maxsteprect)==true)
						{
							if(maxstep<63)
							{
								maxstep++;
							}
							pattern[8][0][0][10][0]=5;
							pattern[8][0][0][10][1]=maxstep;
						}
						if(CheckMouse(mousex, mousey, bpmrect)==true)
						{
							if(bpm<640)
							{
								bpm++;
							}
						}
						if(CheckMouse(mousex, mousey, timedevrect)==true)
						{
							if(timedivision==4)
							{
								timedivision=8;
							}
							else if(timedivision==8)
							{
								timedivision=16;
							}
							else if(timedivision==16)
							{
								timedivision=32;
							}
							else
							{
								timedivision=4;
							}
							pattern[8][0][0][9][0]=4;
							pattern[8][0][0][9][1]=timedivision;
						}
						if(CheckMouse(mousex, mousey, progrect)==true)
						{
							if(selprog[seldev]<dsettings[seldev].maxprog)
							{
								selprog[seldev]++;
								if(dsettings[seldev].mididevice==255)
								{
									fluid_synth_program_change(fluid_synth, dsettings[seldev].midichannel-1,selprog[seldev]);
								}
								else
								{
									wsmidi.ProgramChange(dsettings[seldev].mididevice, dsettings[seldev].midichannel-1, selprog[seldev]);
								}
							}
						}
						if(CheckMouse(mousex, mousey, bankrect)==true)
						{
							if(selbank[seldev]<dsettings[seldev].maxbank)
							{
								selbank[seldev]++;
							}
						}
						if(CheckMouse(mousex, mousey, updown_area)==true)
						{
							if(oktave[seldev]>-2)
							{
								oktave[seldev]--;
							}
							updown_aktiv = false;
						}
						if(isedit==true)
						{
							if(selpatt[0]==seldev and selpatt[1]==selpat[seldev] and selpatt[2]==mousescorex-7 and selpatt[3]==mousescorey-5)
							{
								if(selpatt[3]==15)
								{
									if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][15][0]==6)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][15][1]<dsettings[seldev].maxprog)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][15][1]++;
											selprog[seldev]=pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][15][1];
										}
									}
								}
								if(strcmp(dsettings[seldev].type, "drum")==0)
								{
									if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][0]==1)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2]<127)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2]++;
											volume[seldev]=pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2];
										}
									}
								}
								else if(seldev<8)
								{
									if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]==1 or pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]==2)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]<127)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]++;
											volume[seldev]=pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2];
										}
									}
								}
							}
							if(seldev==8)
							{
								if(selpatt[3]!=255)
								{
									if(selpatt[3]<8)
									{
										if(pattern[8][songtab][selpatt[2]][selpatt[3]][0]==0)
										{
											pattern[8][songtab][selpatt[2]][selpatt[3]][0]=1;
										}
										else
										{
											if(pattern[8][songtab][selpatt[2]][selpatt[3]][1]<16)
											{
												pattern[8][songtab][selpatt[2]][selpatt[3]][1]++;
											}
										}
									}
									if(selpatt[3]==8)
									{
										if(pattern[8][songtab][selpatt[2]][8][0]==0)
										{
											pattern[8][songtab][selpatt[2]][8][0]=2;
											pattern[8][songtab][selpatt[2]][8][1]=bpm/4;
										}
										else
										{
											if(pattern[8][songtab][selpatt[2]][8][1]<255)
											{
												pattern[8][songtab][selpatt[2]][8][1]++;
											}
										}
									}
								}
							}
						}
			        	if(seldev==9)
			        	{
			        		for(int i=0;i<8;i++)
			        		{
			        			if (CheckMouse(mousex, mousey, settings_mididevice_rect[i])==true)
			        			{
			        				if(dsettings[i].mididevice<onPorts-1)
			        				{
			        					dsettings[i].mididevice++;
			        				}
			        				if(dsettings[i].mididevice==255)
			        				{
			        					dsettings[i].mididevice=0;
			        				}
			        				if(dsettings[i].mididevice==254)
			        				{
			        					dsettings[i].mididevice=255;
			        				}
			        			}
			        			if (CheckMouse(mousex, mousey, settings_midichannel_rect[i])==true)
			        			{
			        				if(dsettings[i].midichannel<16)
			        				{
			        					dsettings[i].midichannel++;
			        				}
			        			}
			        		}
							if (CheckMouse(mousex, mousey, midiindev_rect)==true)
							{
								if(midiindevice<inPorts-1)
								{
									midiindevice++;
								}
							}
							if (CheckMouse(mousex, mousey, midiinch_rect)==true)
							{
								if(midiinch<15)
								{
									midiinch++;
								}
							}
							if (CheckMouse(mousex, mousey, midiinclockdev_rect)==true)
							{
								if(midiinclockdevice<inPorts-1)
								{
									midiinclockdevice++;
								}
							}
			        	}

			        }
			        if( event.button.button == SDL_BUTTON_WHEELDOWN )
			        {
						if(CheckMouse(mousex, mousey, volumerect)==true)
						{
							if(volume[seldev]>0)
							{
								volume[seldev]--;
							}
						}
						if(CheckMouse(mousex, mousey, maxsteprect)==true)
						{
							if(maxstep>1)
							{
								maxstep--;
								pattern[8][0][0][10][0]=5;
								pattern[8][0][0][10][1]=maxstep;
							}
						}
						if(CheckMouse(mousex, mousey, bpmrect)==true)
						{
							if(bpm>20)
							{
								bpm--;
							}
						}
						if(CheckMouse(mousex, mousey, timedevrect)==true)
						{
							if(timedivision==32)
							{
								timedivision=16;
							}
							else if(timedivision==16)
							{
								timedivision=8;
							}
							else if(timedivision==8)
							{
								timedivision=4;
							}
							else
							{
								timedivision=32;
							}
							pattern[8][0][0][9][0]=4;
							pattern[8][0][0][9][1]=timedivision;
						}
						if(CheckMouse(mousex, mousey, progrect)==true)
						{
							if(selprog[seldev]>0)
							{
								selprog[seldev]--;
								if(dsettings[seldev].mididevice==255)
								{
									fluid_synth_program_change(fluid_synth, dsettings[seldev].midichannel-1,selprog[seldev]);
								}
								else
								{
									wsmidi.ProgramChange(dsettings[seldev].mididevice, dsettings[seldev].midichannel-1, selprog[seldev]);
								}
							}
						}
						if(CheckMouse(mousex, mousey, bankrect)==true)
						{
							if(selbank[seldev]>0)
							{
								selbank[seldev]--;
							}
						}
						if(CheckMouse(mousex, mousey, updown_area)==true)
						{
							if(oktave[seldev]<7)
							{
								oktave[seldev]++;
							}
							updown_aktiv = false;
						}
						if(isedit==true)
						{
							if(selpatt[0]==seldev and selpatt[1]==selpat[seldev] and selpatt[2]==mousescorex-7 and selpatt[3]==mousescorey-5)
							{
								if(selpatt[3]==15)
								{
									if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][15][0]==6)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][15][1]>0)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][15][1]--;
											selprog[seldev]=pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][15][1];
										}
									}
								}
								if(strcmp(dsettings[seldev].type, "mono")==0)
								{
									if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][0]==1)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2]>0)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2]--;
											volume[seldev]=pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][mousescorey-5][2];
										}
									}
								}
								if(seldev<8)
								{
									if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]==1 or pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][0]==2)
									{
										if(pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]>0)
										{
											pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2]--;
											volume[seldev]=pattern[seldev][selpat[seldev]][selpatt[2]+startanzeigepattern*16][0][2];
										}
									}
								}
							}
							if(seldev==8)
							{
								if(selpatt[3]!=255)
								{
									if(selpatt[3]<8)
									{
										if(pattern[8][songtab][selpatt[2]][selpatt[3]][1]>0)
										{
											pattern[8][songtab][selpatt[2]][selpatt[3]][1]--;
										}
										else
										{
											if(pattern[8][songtab][selpatt[2]][selpatt[3]][0]==1)
											{
												pattern[8][songtab][selpatt[2]][selpatt[3]][0]=0;
											}
										}
									}
									if(selpatt[3]==8)
									{
										if(pattern[8][songtab][selpatt[2]][8][1]>0)
										{
											pattern[8][songtab][selpatt[2]][8][1]--;
										}
										else
										{
											if(pattern[8][songtab][selpatt[2]][8][0]==2)
											{
												pattern[8][songtab][selpatt[2]][8][0]=0;
											}
										}
									}
								}
							}
						}
			        	if(seldev==9)
			        	{
			        		for(int i=0;i<8;i++)
			        		{
			        			if (CheckMouse(mousex, mousey, settings_mididevice_rect[i])==true)
			        			{
			        				if(dsettings[i].mididevice==0)
			        				{
			        					dsettings[i].mididevice=255;
			        				}
			        				else if(dsettings[i].mididevice==255)
			        				{
			        					dsettings[i].mididevice=254;
			        				}
			        				else if(dsettings[i].mididevice==254)
			        				{
			        					dsettings[i].mididevice=254;
			        				}
			        				else if(dsettings[i].mididevice>0)
			        				{
			        					dsettings[i].mididevice--;
			        				}
			        			}
			        			if (CheckMouse(mousex, mousey, settings_midichannel_rect[i])==true)
			        			{
			        				if(dsettings[i].midichannel>1)
			        				{
			        					dsettings[i].midichannel--;
			        				}
			        			}
			        		}
							if (CheckMouse(mousex, mousey, midiindev_rect)==true)
							{
								if(midiindevice>0)
								{
									midiindevice--;
								}
							}
							if (CheckMouse(mousex, mousey, midiinch_rect)==true)
							{
								if(midiinch>0)
								{
									midiinch--;
								}
							}
							if (CheckMouse(mousex, mousey, midiinclockdev_rect)==true)
							{
								if(midiinclockdevice>0)
								{
									midiinclockdevice--;
								}
							}
			        	}
			        }

			        break;
				case SDL_MOUSEBUTTONUP:
			        if( event.button.button == SDL_BUTTON_LEFT )
			        {
						  	stillleftclick = false;
						  	updown_aktiv = false;
						  	leftright_aktiv = false;
						  	selkey = 255;
						// Klaviatur Note Off
						  	if(klavlastnote!=255)
						  	{
								if(seldev==17)
								{
									if(mousescorex>3 and mousescorex<20 and mousescorey>4 and mousescorey<21)
									{
										if(sndbnk[klavlastnote].channel!=-1)
										{
											wsmidi.StopSample(klavlastnote);
										}
									}
								}
								else
								{
							  		
							  		if(dsettings[seldev].mididevice==255)
							  		{
							  			fluid_synth_noteoff(fluid_synth, dsettings[seldev].midichannel-1, klavlastnote);
							  		}
									else if(dsettings[seldev].mididevice==254)
									{
										wsmidi.StopSample(klavlastnote);
									}
							  		else
							  		{
							  			wsmidi.NoteOff(dsettings[seldev].mididevice, dsettings[seldev].midichannel-1, klavlastnote);
							  		}
							  	}
							  	klavlastnote = 255;
						  }
			        }
			        break;
				case SDL_MOUSEMOTION:
					mousex = event.button.x;
					mousey = event.button.y;
					mousescorex = mousex / scorex;
					mousescorey = mousey / scorey;

					if(updown_aktiv==true)
					{
						oktave[seldev]=13-mousescorey;
						if(oktave[seldev]>7)
							oktave[seldev]=7;
						if(oktave[seldev]<-2)
							oktave[seldev]=-2;
					}

					if(leftright_aktiv==true)
					{
						if(seldev<8)
						{
							startanzeigepattern=mousescorex-35;
							if(startanzeigepattern>2)
								startanzeigepattern=2;
							if(startanzeigepattern<0)
								startanzeigepattern=0;
						}
						else if(seldev==8)
						{
							songanzeigepattern=mousescorex-21;
							if(songanzeigepattern>13)
								songanzeigepattern=13;
							if(songanzeigepattern<0)
								songanzeigepattern=0;
						}
					}
				anzeige=false;
				break;
			}
		}

		// MidiIn

		if(midiinmessages.empty()==false)
		{
			cout << int(midiinmessages[0][0]) << ' '  << int(midiinmessages[0][1]) << ' '  << int(midiinmessages[0][2]) << ' ' << endl;

			if(midiinmessages[0][0]==153)
			{
				if(midiinmessages[0][1]==40)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[0].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(0+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==41)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[1].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(1+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==42)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[2].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(2+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==43)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[3].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(3+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==48)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[4].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(4+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==49)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[5].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(5+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==50)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[6].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(6+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==51)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[7].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(7+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==36)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[8].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(8+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==37)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[9].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(9+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==38)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[10].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(10+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==39)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[11].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(11+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==44)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[12].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(12+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==45)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[13].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(13+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==46)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[14].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(14+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==47)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(144+dsettings[0].midichannel-1);
						message.push_back(drumsettings[15].note);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.PlaySample(15+16*(oktave[17]+2));
					}
				}
			}
			if(midiinmessages[0][0]==137)
			{
				if(midiinmessages[0][1]==40)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[0].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(0+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==41)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[1].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(1+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==42)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[2].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(2+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==43)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[3].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(3+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==48)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[4].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(4+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==49)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[5].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(5+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==50)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[6].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(6+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==51)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(1284+dsettings[0].midichannel-1);
						message.push_back(drumsettings[7].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(7+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==36)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[8].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(8+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==37)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[9].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(9+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==38)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[10].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(10+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==39)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[11].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(11+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==44)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[12].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(12+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==45)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[13].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(13+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==46)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[14].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(14+16*(oktave[17]+2));
					}
				}
				if(midiinmessages[0][1]==47)
				{
					if(launchkey_tabmode==0)
					{
						midiout->openPort(dsettings[0].mididevice);
						message.clear();
						message.push_back(128+dsettings[0].midichannel-1);
						message.push_back(drumsettings[15].note);
						message.push_back(0);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
					else if(launchkey_tabmode==1)
					{
						wsmidi.StopSample(15+16*(oktave[17]+2));
					}
				}

			}
// Launchkey Keyboard
			if(seldev<8)
			{
				if(midiinmessages[0][0]==144)
				{
					if(dsettings[seldev].mididevice==255)
					{
						fluid_synth_noteon(fluid_synth, dsettings[seldev].midichannel-1, midiinmessages[0][1], midiinmessages[0][2]);
					}
					else if(dsettings[seldev].mididevice==254)
					{
						wsmidi.PlaySample(midiinmessages[0][1]);
					}
					else
					{
						midiout->openPort(dsettings[seldev].mididevice);
						message.clear();
						message.push_back(144+dsettings[seldev].midichannel-1);
						message.push_back(midiinmessages[0][1]);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
				}
				else if(midiinmessages[0][0]==128)
				{
					if(dsettings[seldev].mididevice==255)
					{
						fluid_synth_noteoff(fluid_synth, dsettings[seldev].midichannel-1, midiinmessages[0][1]);
					}
					else if(dsettings[seldev].mididevice==254)
					{
						wsmidi.StopSample(midiinmessages[0][1]);
					}
					else
					{
						midiout->openPort(dsettings[seldev].mididevice);
						message.clear();
						message.push_back(128+dsettings[seldev].midichannel-1);
						message.push_back(midiinmessages[0][1]);
						message.push_back(midiinmessages[0][2]);
						midiout->sendMessage( &message );
						midiout->closePort();
					}
				}
			}
			if(midiinmessages[0][0]==176)
			{
				if(midiinmessages[0][1]==21)
				{
					bpm = 20 + 2 * midiinmessages[0][2];
				}
				if(midiinmessages[0][1]==28)
				{
					volume[seldev] = midiinmessages[0][2];
				}
				else if(midiinmessages[0][2]==127)
				{
					if(midiinmessages[0][1]==108)
					{
						launchkey_tabmode = 0;
					}
					if(midiinmessages[0][1]==109)
					{
						launchkey_tabmode = 1;
						seldev=17;
					}
					if(midiinmessages[0][1]==106)
					{
						if(seldev==0)
						{
							seldev=8;
						}
						else
						{
							seldev--;
						}
					}
					if(midiinmessages[0][1]==107)
					{
						if(seldev==8)
						{
							seldev=0;
						}
						else
						{
							seldev++;						
						}
					}
					if(midiinmessages[0][1]==104)
					{
						if(seldev==17)
						{
							if(oktave[17]>-2)
							{
								oktave[17]--;
							}
						}
						else if(seldev==8 and songtab>0)
						{
							songtab--;
						}
						else if(seldev<8 and selpat[seldev]>0)
						{
							selpat[seldev]--;
						}
					}
					if(midiinmessages[0][1]==105)
					{
						if(seldev==17)
						{
							if(oktave[17]<5)
							{
								oktave[17]++;
							}
						}
						else if(seldev==8 and songtab<7)
						{
							songtab++;
						}
						else if(seldev<8 and selpat[seldev]<15)
						{
							selpat[seldev]++;
						}
					}
				}
			}

			midiinmessages.erase(midiinmessages.begin());

		}

	}

	cout << "clean up fluidsynth" << endl;

   /* Clean up */
//   fluid_synth_sfunload(fluid_synth,sf2id,true);
//	delete_fluid_synth(fluid_synth);
   delete_fluid_settings(fluid_settings);
   delete_fluid_audio_driver(adriver);

	cout << "clean up MIDI" << endl;
	
	delete midiout;
	delete midiin;
	SDL_Quit();
}
