#include "main.h"

static const char *wildmidi_cfg_ram = " \
drumset 0\n\
\n\
 25	Drum_000/025_Snare_Roll.pat\n\
 26	Drum_000/026_Snap.pat\n\
 27	Drum_000/027_High_Q.pat\n\
 31	Drum_000/031_Sticks.pat\n\
 32	Drum_000/032_Square_Click.pat\n\
 33	Drum_000/033_Metronome_Click.pat\n\
 34	Drum_000/034_Metronome_Bell.pat\n\
 35	Drum_000/035_Kick_1.pat amp=100\n\
 36	Drum_000/036_Kick_2.pat amp=100\n\
 37	Drum_000/037_Stick_Rim.pat\n\
 38	Drum_000/038_Snare_1.pat\n\
 39	Drum_000/039_Clap_Hand.pat amp=100\n\
 40	Drum_000/040_Snare_2.pat\n\
 41	Drum_000/041_Tom_Low_2.pat amp=100\n\
 42	Drum_000/042_Hi-Hat_Closed.pat\n\
 43	Drum_000/043_Tom_Low_1.pat amp=100\n\
 44	Drum_000/044_Hi-Hat_Pedal.pat\n\
 45	Drum_000/045_Tom_Mid_2.pat amp=100\n\
 46	Drum_000/046_Hi-Hat_Open.pat\n\
 47	Drum_000/047_Tom_Mid_1.pat amp=100\n\
 48	Drum_000/048_Tom_High_2.pat amp=100\n\
 49	Drum_000/049_Cymbal_Crash_1.pat\n\
 50	Drum_000/050_Tom_High_1.pat amp=100\n\
 51	Drum_000/051_Cymbal_Ride_1.pat\n\
 52	Drum_000/052_Cymbal_Chinese.pat\n\
 53	Drum_000/053_Cymbal_Ride_Bell.pat amp=100\n\
 54	Drum_000/054_Tombourine.pat\n\
 55	Drum_000/055_Cymbal_Splash.pat\n\
 56	Drum_000/056_Cow_Bell.pat\n\
 57	Drum_000/057_Cymbal_Crash_2.pat\n\
 58	Drum_000/058_Vibra-Slap.pat\n\
 59	Drum_000/059_Cymbal_Ride_2.pat\n\
 60	Drum_000/060_Bongo_High.pat\n\
 61	Drum_000/061_Bongo_Low.pat\n\
 62	Drum_000/062_Conga_High_1_Mute.pat\n\
 63	Drum_000/063_Conga_High_2_Open.pat\n\
 64	Drum_000/064_Conga_Low.pat\n\
 65	Drum_000/065_Timbale_High.pat\n\
 66	Drum_000/066_Timbale_Low.pat\n\
 67	Drum_000/067_Agogo_High.pat\n\
 68	Drum_000/068_Agogo_Low.pat\n\
 69	Drum_000/069_Cabasa.pat amp=100\n\
 70	Drum_000/070_Maracas.pat\n\
 71	Drum_000/071_Whistle_1_High_Short.pat\n\
 72	Drum_000/072_Whistle_2_Low_Long.pat\n\
 73	Drum_000/073_Guiro_1_Short.pat\n\
 74	Drum_000/074_Guiro_2_Long.pat\n\
 75	Drum_000/075_Claves.pat amp=100\n\
 76	Drum_000/076_Wood_Block_1_High.pat\n\
 77	Drum_000/077_Wood_Block_2_Low.pat\n\
 78	Drum_000/078_Cuica_1_Mute.pat amp=100\n\
 79	Drum_000/079_Cuica_2_Open.pat amp=100\n\
 80	Drum_000/080_Triangle_1_Mute.pat\n\
 81	Drum_000/081_Triangle_2_Open.pat\n\
 82	Drum_000/082_Shaker.pat\n\
 84	Drum_000/084_Belltree.pat\n\
\n\
bank 0\n\
\n\
 0	Tone_000/000_Acoustic_Grand_Piano.pat amp=120 pan=center\n\
 1	Tone_000/001_Acoustic_Brite_Piano.pat\n\
 2	Tone_000/002_Electric_Grand_Piano.pat\n\
 4	Tone_000/004_Electric_Piano_1_Rhodes.pat\n\
 5	Tone_000/005_Electric_Piano_2_Chorused_Yamaha_DX.pat\n\
 6	Tone_000/006_Harpsichord.pat\n\
 7	Tone_000/007_Clavinet.pat\n\
 8	Tone_000/008_Celesta.pat\n\
 9	Tone_000/009_Glockenspiel.pat\n\
 13	Tone_000/013_Xylophone.pat\n\
 14	Tone_000/014_Tubular_Bells.pat\n\
 15	Tone_000/015_Dulcimer.pat\n\
 16	Tone_000/016_Hammond_Organ.pat\n\
 19	Tone_000/019_Church_Organ.pat\n\
 21	Tone_000/021_Accordion.pat\n\
 23	Tone_000/023_Tango_Accordion.pat\n\
 24	Tone_000/024_Nylon_Guitar.pat\n\
 25	Tone_000/025_Steel_Guitar.pat\n\
 26	Tone_000/026_Jazz_Guitar.pat\n\
 27	Tone_000/027_Clean_Electric_Guitar.pat\n\
 28	Tone_000/028_Muted_Electric_Guitar.pat\n\
 29	Tone_000/029_Overdriven_Guitar.pat\n\
 30	Tone_000/030_Distortion_Guitar.pat\n\
 32	Tone_000/032_Acoustic_Bass.pat\n\
 33	Tone_000/033_Finger_Bass.pat\n\
 34	Tone_000/034_Pick_Bass.pat\n\
 35	Tone_000/035_Fretless_Bass.pat\n\
 36	Tone_000/036_Slap_Bass_1.pat\n\
 37	Tone_000/037_Slap_Bass_2.pat\n\
 38	Tone_000/038_Synth_Bass_1.pat\n\
 40	Tone_000/040_Violin.pat\n\
 42	Tone_000/042_Cello.pat\n\
 44	Tone_000/044_Tremolo_Strings.pat\n\
 45	Tone_000/045_Pizzicato_Strings.pat\n\
 46	Tone_000/046_Harp.pat\n\
 47	Tone_000/047_Timpani.pat\n\
 48	Tone_000/048_String_Ensemble_1_Marcato.pat\n\
 53	Tone_000/053_Voice_Oohs.pat\n\
 56	Tone_000/056_Trumpet.pat\n\
 57	Tone_000/057_Trombone.pat\n\
 58	Tone_000/058_Tuba.pat\n\
 59	Tone_000/059_Muted_Trumpet.pat\n\
 60	Tone_000/060_French_Horn.pat\n\
 61	Tone_000/061_Brass_Section.pat\n\
 64	Tone_000/064_Soprano_Sax.pat\n\
 65	Tone_000/065_Alto_Sax.pat\n\
 66	Tone_000/066_Tenor_Sax.pat\n\
 67	Tone_000/067_Baritone_Sax.pat\n\
 68	Tone_000/068_Oboe.pat\n\
 69	Tone_000/069_English_Horn.pat\n\
 70	Tone_000/070_Bassoon.pat\n\
 71	Tone_000/071_Clarinet.pat\n\
 72	Tone_000/072_Piccolo.pat\n\
 73	Tone_000/073_Flute.pat\n\
 74	Tone_000/074_Recorder.pat\n\
 75	Tone_000/075_Pan_Flute.pat\n\
 76	Tone_000/076_Bottle_Blow.pat\n\
 79	Tone_000/079_Ocarina.pat\n\
 80	Tone_000/080_Square_Wave.pat\n\
 84	Tone_000/084_Charang.pat\n\
 88	Tone_000/088_New_Age.pat\n\
 94	Tone_000/094_Halo_Pad.pat\n\
 95	Tone_000/095_Sweep_Pad.pat\n\
 98	Tone_000/098_Crystal.pat\n\
 101	Tone_000/101_Goblins--Unicorn.pat\n\
 102	Tone_000/102_Echo_Voice.pat\n\
 104	Tone_000/104_Sitar.pat\n\
 114	Tone_000/114_Steel_Drums.pat\n\
 115	Tone_000/115_Wood_Block.pat\n\
 120	Tone_000/120_Guitar_Fret_Noise.pat\n\
 122	Tone_000/122_Seashore.pat\n\
 125	Tone_000/125_Helicopter.pat\n\
";

//usleep
void usleep(uint32_t us) {
    vTaskDelay(us / portTICK_PERIOD_MS);
}

