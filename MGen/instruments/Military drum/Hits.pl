# Main
library = "True Strike Cinematic Orchestral Percussion" # For which library algorithm is optimized
Type = 0 # Instrument type
poly = 100 # Maximum number of simultaneous voices
n_min = D2

# Instrument parameters
ReplacePitch = "C4" # Replace all notes in track with this pitch
Volume_default = 50 # (direct CC volume, should not be changed by user)
CC_dynamics = 1
CC_ma = 3 # Number of CC steps to moving average (please use only odd numbers)
CC_steps = 1 # Number of CC steps in one note step (please use only odd numbers)

# Controls
KswGroup = "C2: Snares on", "C#2: Snares off"

# Initial setup
Snares on = 101

# Tremolo adaptor
trem_maxlen = 100 # Maximum length of one note inside tremolo for detection
trem_min_repeats = 3 # Minimum number of note repeats to detect tremolo
trem_dyn_range = 0-100 # (0-100) Percent of tremolo dynamics compared to sustain notes
trem_replace = "F4" # Replace tremolo notes with this note
trem_maxint = 127 # Maximum interval between notes of a trill in semitones to be considered a tremolo
trem_end = 1 # 0 - normal ending, 1 - cutoff (need hit), 2 - ending hit
trem_len = 7000 # Tremolo maximum sample length in ms
trem_end = 1 # 0 - normal ending, 1 - cutoff (need hit), 2 - ending hit

# Randomization
rnd_vel = 8 # Randomize note velocity not greater than this percent
rand_pos = 8-0 # Randomize note starts-ends not greater than percent of note length 
rand_pos_max = 40-0 # Maximum shift in ms (start-end)

