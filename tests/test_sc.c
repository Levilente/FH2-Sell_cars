#include <stdio.h>
#include <string.h>
#include "../src/sc_text.h"
static int fails;
#define CHECK(c) do{ if(!(c)){ printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } }while(0)
int main(void){
    char m[600]; ScBuf b;
    sc_begin(&b,m,sizeof m); sc_addn_grouped(&b,1234567); CHECK(!strcmp(m,"1,234,567"));
    sc_begin(&b,m,sizeof m); sc_addn_grouped(&b,0); CHECK(!strcmp(m,"0"));
    sc_begin(&b,m,sizeof m); sc_addn_grouped(&b,999); CHECK(!strcmp(m,"999"));
    sc_begin(&b,m,sizeof m); sc_addn_grouped(&b,1000); CHECK(!strcmp(m,"1,000"));
    sc_begin(&b,m,sizeof m); sc_addn(&b,-42); CHECK(!strcmp(m,"-42"));
    sc_price_sql(m,sizeof m,32,50,30);
    CHECK(strstr(m,"Profile0_Career_Garage WHERE Id=32"));
    CHECK(strstr(m,"GarageId=32"));
    CHECK(strstr(m,")),0)*50+IFNULL("));
    CHECK(strstr(m,"),0)*30)/100 AS INTEGER) AS V"));
    CHECK(strlen(m)<sizeof m-1);
    char small[10]; sc_price_sql(small,sizeof small,1,1,1); CHECK(strlen(small)==9);   /* truncates, never overflows */
    CHECK(sc_clamp_pct(50,10)==50); CHECK(sc_clamp_pct(-1,10)==10); CHECK(sc_clamp_pct(101,10)==10); CHECK(sc_clamp_pct(0,10)==0);
    uint16_t w[8]; sc_widen(w,8,"Sell car!!!"); CHECK(w[0]=='S'&&w[6]=='a'&&w[7]==0);
    printf(fails?"FAILED %d\n":"OK\n",fails);
    return fails!=0;
}
