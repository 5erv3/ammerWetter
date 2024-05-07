
void Animation_RAIN(){
    // 80 OK
    Serial.println("ANIMAZIONE RAIN");
    unsigned C_HUE = 42000; int C_SAT = 255; int C_VALUE = MaxValue/2;
    unsigned E_HUE = 42000; int E_SAT = 200; int E_VALUE = MaxValue;   
    String a;

    a =    "1010";
    a=a+  "0000000";
    a=a+ "0000000000";
    a=a+  "0000000";
    a=a+   "0000";
    
   FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "0000";
    a=a+  "0100101";
    a=a+ "0000000000";
    a=a+  "0000000";
    a=a+   "0000";
    
    FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "0000";
    a=a+  "0000000";
    a=a+ "0010010010";
    a=a+  "0000000";
    a=a+   "0000";
    
   FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "0000";
    a=a+  "0000000";
    a=a+ "0000000000";
    a=a+  "0100101";
    a=a+   "0000";
    
  FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "0000";
    a=a+  "0000000";
    a=a+ "0000000000";
    a=a+  "0000000";
    a=a+   "1001";
    
   FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
    a =    "0000";
    a=a+  "0000000";
    a=a+ "0000000000";
    a=a+  "0000000";
    a=a+   "0000";
    
   FillColor(a,C_HUE,C_SAT,C_VALUE,E_HUE, E_SAT,E_VALUE);
   delay(250);
// ------------------------------------------------
 
}
