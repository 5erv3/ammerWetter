
void Animation_SNOW(){
    //71

    Serial.println("ANIMAZIONE RAIN");
    unsigned C_HUE = 56500; int C_SAT = 200; int C_VALUE = MaxValue/2;
    unsigned E_HUE = 48000; int E_SAT = 200; int E_VALUE = MaxValue;   
    String a;

    a =    "1010";
    a=a+  "1010101";
    a=a+ "1010001010";
    a=a+  "10110";
    a=a+   "1010";
    
   FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "1100";
    a=a+  "0100101";
    a=a+ "0110110101";
    a=a+  "0010010";
    a=a+   "1011";
    
    FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "0110";
    a=a+  "1101001";
    a=a+ "1001110111";
    a=a+  "0110100";
    a=a+   "1001";
    
   FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "1100";
    a=a+  "1001011";
    a=a+ "1010011001";
    a=a+  "1100101";
    a=a+   "0111";
    
  FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "1101";
    a=a+  "1100010";
    a=a+ "1101010001";
    a=a+  "1110001";
    a=a+   "0001";
    
   FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "1000";
    a=a+  "0110010";
    a=a+ "1000100010";
    a=a+  "1011001";
    a=a+   "0100";
    
   FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
}
