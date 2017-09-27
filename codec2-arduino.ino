#include "codec2.h"

//#define TESTING

#define STORAGE_CAPABLE

#ifdef STORAGE_CAPABLE
  #include <SD.h>

  Sd2Card card;
  SdVolume volume;
  SdFile root;
  
  // M0 feather with SD wing
//  #define SD_CS 10  
  // Adalogger SD
//  #define SD_CS 4
  // nRF52 feather with SD wing
  #define SD_CS 11
#endif

bool storageReady=false;

const int mode=CODEC2_MODE_700B;
const int natural=1;

CODEC2 *codec2;
int nsam, nbit, nbyte, i, frames, bits_proc, bit_errors, error_mode;
int nstart_bit, nend_bit, bit_rate;
short *buf;
unsigned char *bits;
float *softdec_bits;

void setup()
{
  Serial.begin(19200);

  unsigned long startTime=millis();
  while(!Serial && (startTime+(10*1000) > millis())) {}

#ifdef STORAGE_CAPABLE
  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    storageReady=false;
    #ifdef TESTING
      Serial.println("SD not found");
      Serial.println("storage disabled");    
    #endif
  } else {
    #ifdef TESTING
      Serial.println("SD card wiring is correct and a card is present.");
    
    // print the type of card
    Serial.print("\nCard type: ");
    switch (card.type()) {
      case SD_CARD_TYPE_SD1:
        Serial.println("SD1");
        break;
      case SD_CARD_TYPE_SD2:
        Serial.println("SD2");
        break;
      case SD_CARD_TYPE_SDHC:
        Serial.println("SDHC");
        break;
      default:
        Serial.println("Unknown");
    }
    #endif

    if(volume.init(card))
    {        
      uint32_t volumesize;
      #ifdef TESTING
        Serial.print("\nVolume type is FAT");
        Serial.println(volume.fatType(), DEC);
        Serial.println();
      #endif

      storageReady=true;
      #ifdef TESTING
        Serial.println("storage enabled");
    
        volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
        volumesize *= volume.clusterCount();       // we'll have a lot of clusters
        volumesize *= 512;                            // SD card blocks are always 512 bytes
        Serial.print("Volume size (bytes): ");
        Serial.println(volumesize);
        Serial.print("Volume size (Kbytes): ");
        volumesize /= 1024;
        Serial.println(volumesize);
        Serial.print("Volume size (Mbytes): ");
        volumesize /= 1024;
        Serial.println(volumesize);
      
        Serial.println("\nFiles found on the card (name, date and size in bytes): ");
      #endif
      
      root.openRoot(volume);
    
      // list all files in the card with date and size
      #ifdef TESTING
        root.ls(LS_R | LS_DATE | LS_SIZE);
      #endif
    }
    else
    {
      Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
      storageReady=false;
      Serial.println("storage disbled");
    }
  }
#endif  

  codec2 = codec2_create(mode);
  nsam = codec2_samples_per_frame(codec2);
  nbit = codec2_bits_per_frame(codec2);
  buf = (short*)malloc(nsam*sizeof(short));
  nbyte = (nbit + 7) / 8;
  bits = (unsigned char*)malloc(nbyte*sizeof(char));
  softdec_bits = (float*)malloc(nbit*sizeof(float));
  frames = bit_errors = bits_proc = 0;
  nstart_bit = 0;
  nend_bit = nbit-1;

  codec2_set_natural_or_gray(codec2, !natural);  

  if(storageReady)
  {
    recitation();    
  }

  codec2_destroy(codec2);
}

void loop()
{
}

void recitation()
{
  #ifdef TESTING
    Serial.println("Begin recitation");
  #endif
  uint8_t c2Buf[nbyte];
  uint8_t maxlen = sizeof(c2Buf);
  uint8_t offset=0;

  int16_t audioBuf[nsam];

  SD.begin(SD_CS);
  File c2File = SD.open("TEST700B.C2");
  if (c2File) {
    #ifdef TESTING
      Serial.println("Reciting Codec2 file");
    #endif

    // read from the file until there's nothing else in it:
    while (c2File.available()) {
      #ifdef TESTING
        Serial.print("~");
      #endif

      offset=0;
      for(int x=0; x<maxlen; x++) {
        if(!c2File.available()) {
          #ifdef TESTING
            Serial.println("EOF");
          #endif
          break;
        }
        
        c2Buf[offset++] = c2File.read();
      }

      if(offset!=nbyte)
      {
        break;
      }      

      codec2_decode(codec2, audioBuf, c2Buf);

      for(int x=0; x<nsam; x++)
      {
        uint16_t sample=audioBuf[x];
        char lo = sample & 0xFF;
        char hi = sample >> 8;
        Serial.print(lo);
        Serial.print(hi);
      }
    }
    
    // close the file:
    c2File.close();
    #ifdef TESTING
      Serial.println("Recitation complete.");
    #endif    
  } else {
    // if the file didn't open, print an error:
    Serial.println("Error opening Codec2 file");
  }
}

