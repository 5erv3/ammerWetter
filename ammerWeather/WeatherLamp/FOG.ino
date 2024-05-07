
void Animation_FOG(){
    // 45 OK
    Serial.println("ANIMAZIONE FOG");
    unsigned C_HUE = 58552; int C_SAT = 20; int C_VALUE = MaxValue;
    int Variazione = MaxValue*70/100;
    Serial.print(Variazione);
    Serial.print("MAX VALUE: ");
    Serial.println(MaxValue);
     for (int q= (C_VALUE-Variazione); q< C_VALUE;  q++) {
        Serial.println(q);
        FillColor("",C_HUE,C_SAT,q,0,0,0);
        delay(10);
     }

    for (int q=C_VALUE; q> (C_VALUE-Variazione); q--) {
        FillColor("",C_HUE,C_SAT,q,0,0,0);
        delay(10);
     }
 

   
}
