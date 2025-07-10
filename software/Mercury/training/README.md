# Neural Amp Model Training for Mercury

To train your own neural models for use with Mercury/Funbox, use the included model definition in the 
"model_pico.json" file with the Neural Amp Modeler local trainer. The Pico models (an unofficial NAM model name) use the wavenet
architecture, and are the same as Nano models except they have 2 channels on the first layer (instead of 4). 
The input size of the 2nd layer is also changed to 2 to match the 1st layer output.

Once you have trained a pico model, copy and paste the weights into the "model_data_nam.h" file. No other info is used
from the trained model besides the "weights" field. Recompile Mercury to get a new binary to upload to the Daisy Seed 
with the updated models. 

Note: These pico models are fully compatible with the official NAM plugin. You can test them out there prior to
uploading to Funbox. Because the model size is smaller than the usual NAM model, results may vary. Most loss values I 
obtained for amplifiers were between 0.01 and 0.03.