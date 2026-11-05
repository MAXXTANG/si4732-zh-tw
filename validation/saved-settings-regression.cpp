
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
struct EEPROMBuffer {
 std::array<uint8_t,2048> data; int commits=0;
 bool begin(int n){return n==2048;} void end(){}
 uint8_t read(int i){return data.at(i);} void write(int i,uint8_t v){data.at(i)=v;}
 bool commit(){++commits; return true;}
} EEPROM;
struct {int vol; uint16_t frequency=10390; int step;
 void setVolume(int v){vol=v;} int getVolume(){return vol;} uint16_t getFrequency(){return frequency;}
 void setFrequencyStep(int v){step=v;} void setSSBAudioBandwidth(int){} void setSSBSidebandCutoffFilter(int){}
 void setSSBBfo(int){} void setFmBandwidth(int){} void setBandwidth(int,int){}
} rx;
struct { void println(const char*){} } Serial;
void delay(int){} void useBand(){} void loadSSB(){}
enum {FM_BAND_TYPE,MW_BAND_TYPE,SW_BAND_TYPE,LW_BAND_TYPE};
enum {FM,LSB,USB,AM};
constexpr int storenumber=50, app_id=47, eeprom_address=0, EEPROM_SIZE=2048, DISPLAY_SETTINGS_VERSION=1, MIN_BACKLIGHT=26;
int station[51], volume=35, currentBFO=0, lcdofftime=0, brightness=255;
uint16_t currentFrequency=10390;
bool fmRDS=false, ssbLoaded=false, cmdseek=false;
char bfo[18]; double CONV_FACTOR=1.71;
typedef struct
{
  uint8_t idx;       // SI473X device bandwidth index
  const char *desc;  // bandwidth description
} Bandwidth;

int8_t bwIdxSSB = 4;
const int8_t maxSsbBw = 5;
Bandwidth bandwidthSSB[] = {
  { 4, "0.5" },
  { 5, "1.0" },
  { 0, "1.2" },
  { 1, "2.2" },
  { 2, "3.0" },
  { 3, "4.0" }
};
const int lastBandwidthSSB = (sizeof bandwidthSSB / sizeof(Bandwidth)) - 1;

int8_t bwIdxAM = 4;
const int8_t maxAmBw = 6;
Bandwidth bandwidthAM[] = {
  { 4, "1.0" },
  { 5, "1.8" },
  { 3, "2.0" },
  { 6, "2.5" },
  { 2, "3.0" },
  { 1, "4.0" },
  { 0, "6.0" }
};
const int lastBandwidthAM = (sizeof bandwidthAM / sizeof(Bandwidth)) - 1;

int8_t bwIdxFM = 0;
const int8_t maxFmBw = 4;

Bandwidth bandwidthFM[] = {
  { 0, "AUT" },  // Automatic - default
  { 1, "110" },  // Force wide (110 kHz) channel filter.
  { 2, " 84" },
  { 3, " 60" },
  { 4, " 40" }
};
const int lastBandwidthFM = (sizeof bandwidthFM / sizeof(Bandwidth)) - 1;

int tabAmStep[] = { 1, 9, 50, 100 };  //am步进
const int lastAmStep = (sizeof tabAmStep / sizeof(int)) - 1;
int idxAmStep = 3;

int tabFmStep[] = { 10, 20, 100 };  //fm步进
const int lastFmStep = (sizeof tabFmStep / sizeof(int)) - 1;
int idxFmStep = 1;

int tabSwStep[] = { 1, 5, 10, 50, 100 };  //sw步进
const int lastSwStep = (sizeof tabSwStep / sizeof(int)) - 1;
int idxSwStep = 1;


uint16_t currentStepIdx = 1;


const char *bandModeDesc[] = { "FM ", "LSB", "USB", "AM " };
const int lastBandModeDesc = (sizeof bandModeDesc / sizeof(char *)) - 1;
uint8_t currentMode = FM;


/**
 *  Band data structure
 */
typedef struct
{
  const char *bandName;   // Band description
  uint8_t bandType;       // Band type (FM, MW or SW)
  uint16_t minimumFreq;   // Minimum frequency of the band
  uint16_t maximumFreq;   // maximum frequency of the band
  uint16_t currentFreq;   // Default frequency or current frequency
  int8_t currentStepIdx;  // Idex of tabStepAM:  Defeult frequency step (See tabStepAM)
  int8_t bandwidthIdx;    // Index of the table bandwidthFM, bandwidthAM or bandwidthSSB;
} Band;


Band band[] = {
  { "VHF", FM_BAND_TYPE, 6400, 10800, 10390, 0, 0 },
  { "MW1", MW_BAND_TYPE, 153, 1719, 810, 1, 4 },
  { "MW2", MW_BAND_TYPE, 531, 1701, 783, 1, 4 },
  { "MW3", MW_BAND_TYPE, 1701, 3501, 2500, 1, 4 },
  { "80M", SW_BAND_TYPE, 3500, 4000, 3700, 1, 4 },  //原为 MW_BAND_TYPE
  { "SW1", SW_BAND_TYPE, 4000, 5500, 4885, 1, 4 },
  { "SW2", SW_BAND_TYPE, 5500, 6500, 6000, 1, 4 },
  { "40M", SW_BAND_TYPE, 6500, 7300, 7100, 1, 4 },
  { "SW3", SW_BAND_TYPE, 7200, 8000, 7200, 1, 4 },
  { "SW4", SW_BAND_TYPE, 9000, 11000, 9500, 1, 4 },
  { "SW5", SW_BAND_TYPE, 11100, 13000, 11900, 1, 4 },
  { "SW6", SW_BAND_TYPE, 13000, 14000, 13500, 1, 4 },
  { "20M", SW_BAND_TYPE, 14000, 15000, 14200, 1, 4 },
  { "SW7", SW_BAND_TYPE, 15000, 17000, 15300, 1, 4 },
  { "SW8", SW_BAND_TYPE, 17000, 18000, 17500, 1, 4 },
  { "15M", SW_BAND_TYPE, 20000, 21400, 21100, 1, 4 },
  { "SW9", SW_BAND_TYPE, 21400, 22800, 21500, 1, 4 },
  { "CB ", SW_BAND_TYPE, 26000, 28000, 27500, 1, 4 },
  { "10M", SW_BAND_TYPE, 28000, 30000, 28400, 1, 4 },
  { "ALL", SW_BAND_TYPE, 150, 30000, 15000, 1, 4 }  // All band. LW, MW and SW (from 150kHz to 30MHz)
};

const int lastBand = (sizeof band / sizeof(Band)) - 1;
int bandIdx = 0;
void savestation(int n)  //存储电台
{
  if (n < 1 || n > storenumber || !EEPROM.begin(EEPROM_SIZE)) return;

  EEPROM.write(eeprom_address + 100 + n * 15 + 1, bandIdx);                       // Stores the current band
  EEPROM.write(eeprom_address + 100 + n * 15 + 2, currentMode);                   // Stores the current Mode (FM / AM / SSB)
  EEPROM.write(eeprom_address + 100 + n * 15 + 3, (rx.getFrequency() >> 8));      // stores the current Frequency HIGH byte for the band
  EEPROM.write(eeprom_address + 100 + n * 15 + 4, (rx.getFrequency() & 0xFF));    //stores the current Frequency LOW byte for the band
  EEPROM.write(eeprom_address + 100 + n * 15 + 5, band[bandIdx].currentStepIdx);  // Stores current step of the band
  EEPROM.write(eeprom_address + 100 + n * 15 + 6, band[bandIdx].bandwidthIdx);    // table index (direct position) of bandwidth
  EEPROM.write(eeprom_address + 100 + n * 15 + 7, n);                       //存储当前电台号码

  EEPROM.commit();
  EEPROM.end();
}

bool validSavedMode(uint8_t index, uint8_t mode) {
  if (index > lastBand || mode > lastBandModeDesc) return false;
  return (band[index].bandType == FM_BAND_TYPE) == (mode == FM);
}

bool readBandSettings(uint8_t index, uint8_t mode, int address) {
  if (!validSavedMode(index, mode)) return false;
  Band &entry = band[index];
  uint16_t frequency = (EEPROM.read(address) << 8) | EEPROM.read(address + 1);
  uint8_t step = EEPROM.read(address + 2);
  uint8_t width = EEPROM.read(address + 3);
  int maxStep = lastSwStep;
  if (entry.bandType == FM_BAND_TYPE) maxStep = lastFmStep;
  if (entry.bandType == MW_BAND_TYPE) maxStep = lastAmStep;
  int maxWidth = lastBandwidthAM;
  if (mode == FM) maxWidth = lastBandwidthFM;
  if (mode == LSB || mode == USB) maxWidth = lastBandwidthSSB;
  if (frequency < entry.minimumFreq || frequency > entry.maximumFreq ||
      step > maxStep || width > maxWidth) return false;
  entry.currentFreq = frequency;
  entry.currentStepIdx = step;
  entry.bandwidthIdx = width;
  return true;
}

void readstation(int n)
{
  if (n < 1 || n > storenumber || !EEPROM.begin(EEPROM_SIZE)) return;
  int base = eeprom_address + 100 + n * 15;
  uint8_t savedBand = EEPROM.read(base + 1);
  uint8_t savedMode = EEPROM.read(base + 2);
  bool valid = EEPROM.read(base + 7) == n && readBandSettings(savedBand, savedMode, base + 3);
  EEPROM.end();
  if (!valid) return;
  bandIdx = savedBand;
  currentMode = savedMode;
  if (currentMode != USB && currentMode != LSB) ssbLoaded = false;

  currentFrequency = band[bandIdx].currentFreq;

  if (band[bandIdx].bandType == FM_BAND_TYPE) {
    currentStepIdx = idxFmStep = band[bandIdx].currentStepIdx;
    rx.setFrequencyStep(tabFmStep[currentStepIdx]);
  } else if (band[bandIdx].bandType == SW_BAND_TYPE) {
    currentStepIdx = idxSwStep = band[bandIdx].currentStepIdx;
    rx.setFrequencyStep(tabSwStep[currentStepIdx]);
  } else {
    currentStepIdx = idxAmStep = band[bandIdx].currentStepIdx;
    rx.setFrequencyStep(tabAmStep[currentStepIdx]);
  }

  int bwIdx = band[bandIdx].bandwidthIdx;

  if (currentMode == LSB || currentMode == USB) {
    loadSSB();
    bwIdxSSB = (bwIdx > 5) ? 5 : bwIdx;
    rx.setSSBAudioBandwidth(bandwidthSSB[bwIdxSSB].idx);
    // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
    if (bandwidthSSB[bwIdxSSB].idx == 0 || bandwidthSSB[bwIdxSSB].idx == 4 || bandwidthSSB[bwIdxSSB].idx == 5)
      rx.setSSBSidebandCutoffFilter(0);
    else
      rx.setSSBSidebandCutoffFilter(1);
    rx.setSSBBfo(currentBFO);
  } else if (currentMode == AM) {
    bwIdxAM = bwIdx;
    rx.setBandwidth(bandwidthAM[bwIdxAM].idx, 1);
  } else {
    bwIdxFM = bwIdx;
    rx.setFmBandwidth(bandwidthFM[bwIdxFM].idx);
  }

  if (currentBFO > 0)
    sprintf(bfo, "+%4.4d", currentBFO);
  else
    sprintf(bfo, "%4.4d", currentBFO);

  useBand();

  delay(50);
}

bool saveAllReceiverInformation()  // Commit header and band records together.
{
  int addr_offset;

  if (!EEPROM.begin(EEPROM_SIZE)) return false;

  EEPROM.write(eeprom_address, app_id);              // stores the app id;
  EEPROM.write(eeprom_address + 1, rx.getVolume());  // stores the current Volume
  EEPROM.write(eeprom_address + 2, bandIdx);         // Stores the current band
  EEPROM.write(eeprom_address + 3, fmRDS);
  EEPROM.write(eeprom_address + 4, currentMode);  // Stores the current Mode (FM / AM / SSB)
  EEPROM.write(eeprom_address + 5, currentBFO >> 8);
  EEPROM.write(eeprom_address + 6, currentBFO & 0XFF);
  EEPROM.write(eeprom_address + 7, brightness);
  EEPROM.write(eeprom_address + 8, lcdofftime);
  EEPROM.write(eeprom_address + 12, DISPLAY_SETTINGS_VERSION);
  EEPROM.write(eeprom_address + 9, cmdseek ? 1 : 0);  //存储 搜索
  //if(!cmdseek) EEPROM.write(eeprom_address + 79,0);
  //电池校准系数 10


  addr_offset = 15;
  band[bandIdx].currentFreq = currentFrequency;

  for (int i = 0; i <= lastBand; i++) {
    EEPROM.write(addr_offset++, (band[i].currentFreq >> 8));    // stores the current Frequency HIGH byte for the band
    EEPROM.write(addr_offset++, (band[i].currentFreq & 0xFF));  // stores the current Frequency LOW byte for the band
    EEPROM.write(addr_offset++, band[i].currentStepIdx);        // Stores current step of the band
    EEPROM.write(addr_offset++, band[i].bandwidthIdx);          // table index (direct position) of bandwidth
  }

  bool saved = EEPROM.commit();
  EEPROM.end();
  return saved;
}

void readAllReceiverInformation()
{
  int bwIdx;
  rx.setVolume(volume);
  if (!EEPROM.begin(EEPROM_SIZE)) return;
  uint16_t calibration = (EEPROM.read(eeprom_address + 10) << 8) | EEPROM.read(eeprom_address + 11);
  if (calibration > 0 && calibration <= 10000) CONV_FACTOR = calibration / 1000.0;
  uint8_t savedBand = EEPROM.read(eeprom_address + 2);
  uint8_t savedMode = EEPROM.read(eeprom_address + 4);
  uint8_t savedVolume = EEPROM.read(eeprom_address + 1);
  // Older images also set app_id after a preset save, even with an empty receiver header.
  if (EEPROM.read(eeprom_address) != app_id || savedVolume > 63 || !validSavedMode(savedBand, savedMode)) {
    EEPROM.end();
    Serial.println("[ZH-TW 0.2.1] settings=defaults");
    return;
  }
  bandIdx = savedBand;
  currentMode = savedMode;
  fmRDS = EEPROM.read(eeprom_address + 3) == 1;
  currentBFO = EEPROM.read(eeprom_address + 5) << 8;
  currentBFO |= EEPROM.read(eeprom_address + 6);
  // Preserve radio settings from v0.1, and start its first upgraded boot fully lit.
  if (EEPROM.read(eeprom_address + 12) == DISPLAY_SETTINGS_VERSION) {
    uint8_t savedLevel = EEPROM.read(eeprom_address + 7);
    uint8_t savedTimeout = EEPROM.read(eeprom_address + 8);
    if (savedLevel >= MIN_BACKLIGHT) brightness = savedLevel;
    if (savedTimeout <= 99) lcdofftime = savedTimeout;
  }
  cmdseek = EEPROM.read(eeprom_address + 9) == 1;
  for (int i = 0; i <= lastBand; i++) {
    uint8_t mode = (band[i].bandType == FM_BAND_TYPE) ? FM : AM;
    if (i == bandIdx) mode = currentMode;
    readBandSettings(i, mode, eeprom_address + 15 + 4 * i);
  }

  EEPROM.end();

  currentFrequency = band[bandIdx].currentFreq;

  if (band[bandIdx].bandType == FM_BAND_TYPE) {
    currentStepIdx = idxFmStep = band[bandIdx].currentStepIdx;
    rx.setFrequencyStep(tabFmStep[currentStepIdx]);
  } else if (band[bandIdx].bandType == MW_BAND_TYPE) {
    currentStepIdx = idxAmStep = band[bandIdx].currentStepIdx;
    rx.setFrequencyStep(tabAmStep[currentStepIdx]);
  } else if (band[bandIdx].bandType == SW_BAND_TYPE){
    currentStepIdx = idxSwStep = band[bandIdx].currentStepIdx;
    rx.setFrequencyStep(tabSwStep[currentStepIdx]);
  }
  

  bwIdx = band[bandIdx].bandwidthIdx;

  if (currentMode == LSB || currentMode == USB) {
    loadSSB();
    bwIdxSSB = (bwIdx > 5) ? 5 : bwIdx;
    rx.setSSBAudioBandwidth(bandwidthSSB[bwIdxSSB].idx);
    // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
    if (bandwidthSSB[bwIdxSSB].idx == 0 || bandwidthSSB[bwIdxSSB].idx == 4 || bandwidthSSB[bwIdxSSB].idx == 5)
      rx.setSSBSidebandCutoffFilter(0);
    else
      rx.setSSBSidebandCutoffFilter(1);
    rx.setSSBBfo(currentBFO);
  } else if (currentMode == AM) {
    bwIdxAM = bwIdx;
    rx.setBandwidth(bandwidthAM[bwIdxAM].idx, 1);
  } else {
    bwIdxFM = bwIdx;
    rx.setFmBandwidth(bandwidthFM[bwIdxFM].idx);
  }

  if (currentBFO > 0)
    sprintf(bfo, "+%4.4d", currentBFO);
  else
    sprintf(bfo, "%4.4d", currentBFO);

  delay(50);
  rx.setVolume(savedVolume);
}

int readsavednumcount(void)  //将存储的电台号码读取进 station[]数组 返回存储个数count_station
{
  int count_station = 0;
  int value;
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 1; i <= storenumber; i++) {
    value = EEPROM.read(eeprom_address + 100 + i * 15 + 7);  //将存储的savenum 读取出

    if (value == i)
    {
      count_station++;
      station[count_station] = value;
    }
  }
  EEPROM.end();
  return count_station;
}

int main(){
 // Recreate the invalid 47/255 header using synthetic data suitable for sharing.
 EEPROM.data.fill(255);
 EEPROM.data[0]=app_id;
 const auto invalidSettings=EEPROM.data;
 readAllReceiverInformation();
 assert(EEPROM.data==invalidSettings && bandIdx==0 && currentMode==FM && rx.vol==35 && lcdofftime==0 && brightness==255);
 EEPROM.data.fill(255);
 readAllReceiverInformation();
 assert(bandIdx==0 && rx.vol==35);
 for(int n=1;n<=50;n++) savestation(n);
 assert(EEPROM.data[0]==255 && readsavednumcount()==50 && station[50]==50);
 readstation(50);
 assert(bandIdx==0 && band[0].currentFreq==10390);
 auto before=EEPROM.data;
 int commits = EEPROM.commits;
 rx.vol=26; brightness=200; lcdofftime=12;
 assert(saveAllReceiverInformation() && EEPROM.commits==commits+1 && std::equal(before.begin()+100,before.end(),EEPROM.data.begin()+100));
 brightness=255; lcdofftime=0; rx.vol=0;
 readAllReceiverInformation();
 assert(rx.vol==26 && brightness==200 && lcdofftime==12);
 EEPROM.data[12]=255; brightness=255; lcdofftime=0;
 readAllReceiverInformation();
 assert(rx.vol==26 && brightness==255 && lcdofftime==0);
 EEPROM.data[17]=255; EEPROM.data[18]=255;
 assert(!readBandSettings(0,FM,15) && band[0].currentStepIdx==0 && band[0].bandwidthIdx==0);
 readAllReceiverInformation(); assert(rx.vol==26 && band[0].currentStepIdx==0);
 assert(!validSavedMode(255,FM) && !validSavedMode(0,255) && !validSavedMode(0,AM) && !validSavedMode(1,FM));
 int offset=100+50*15;
 EEPROM.data[offset+1]=255; readstation(50); assert(bandIdx==0);
 EEPROM.data[offset+1]=4; EEPROM.data[offset+2]=USB;
 EEPROM.data[offset+3]=3700>>8; EEPROM.data[offset+4]=3700 & 255;
 EEPROM.data[offset+5]=4; EEPROM.data[offset+6]=5;
 readstation(50); assert(bandIdx==4 && currentMode==USB && rx.step==100);
 assert(!readBandSettings(4,USB,15));
 std::puts("PASS: synthetic 47/255 record, erased settings, all 50 presets, single global commit, preset preservation, valid current and legacy settings, invalid indices, shortwave step and width");
}
