namespace soundsketch
{
	class Soundsketch
	{
		public:
			enum Sw
			{
				FOOTSWITCH_1 = 1,
				FOOTSWITCH_2 = 0,
				SWITCH_1_UP = 3,
				SWITCH_1_DOWN = 2, 
				SWITCH_2_UP = 5,
				SWITCH_2_DOWN = 4, 
				SWITCH_3_UP = 7,
				SWITCH_3_DOWN = 6
			};

			enum Knob
			{
				KNOB_1 = 0,
				KNOB_2 = 1,
				KNOB_3 = 2,
				KNOB_4 = 3,
				KNOB_5 = 4,
				KNOB_6 = 5,
                //EXPRESSION = 6 // (this is handled by referencing "hw.expression" in program code)
			};
			
			enum LED
			{
				LED_1 = 22,
				LED_2 = 23
			};
       
                        // Note: MIDI Input is available on v2/v3 board by pin 37, D30
                        //       MIDI Out is available on v2 board by pin 36, D29

	};
}
