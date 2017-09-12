#include "fun_head.h"

MODELBEGIN




EQUATION("ComputeBill")
/*
Compute the bill for the calling object.
Here you can set different computational systems
*/

v[0]=V("BillingSystem");

if(v[0]==1)
 END_EQUATION(V_CHEAT("LinearBilling",c) ); //return the value of the function computed as if the object 'c' had asked its computation.

plog("\nError in ComputeBill. The code of the billing system is not associated to any function.\n");
INTERACT("Error in ComputeBill", v[0]);

RESULT(-1 )



EQUATION("LinearBilling")
/*
Bill linearly proportional to the consumption
*/

v[0]=V("UnitCost");
v[1]=VS(c,"CumulatedWater");
RESULT(v[0]*v[1] )




EQUATION("WaterBill")
/*
Cost of water consumed in the last period.
It is updated every DaysBill time steps, during the other steps it remains constant, just decreaesing the counter.

Alternatively, it computes the bill with the selected procedure
*/

v[0]=V("CounterDays")-1;
if(v[0]>0)
 {
  INCR("CounterDays",-1);
  END_EQUATION(VL("WaterBill",1));
 }

v[1]=V("ComputeBill"); 
v[4]=V("DaysBill");
WRITE("CounterDays",v[4]);
WRITEL("CumulatedWater",0, t);
RESULT(v[1] )


EQUATION("WaterSaving")
/*
Multiplier to "natural" water demand representing the effort of the hh to vary the consumption because of:
1) social pressure from neighbours
2) variation of the water bill
3) relevance of the water bill in relation to income
*/
v[0]=v[1]=0;

CYCLE_LINK(curl)
 {
  cur2=LINKTO(curl);
  v[0]+=VLS(cur2,"WaterSaving",1);
  v[1]++;
 }
v[2]=v[0]/v[1];

v[3]=VL("WaterSaving",1);

v[4]=V("wSocialEffect");

v[6]=V("wLongTermBill");
v[8]=VL("LongTermBill",1);
v[9]=VL("WaterBill",1);
v[21]=v[8]/v[9];

v[20]=V("wIncomeBill");
v[10]=V("WaterBill"); 
v[11]=V("Income");

v[22]=(v[11]-v[10])/v[11]; 

v[30]=(1-v[4]-v[6]-v[20])*v[3]+v[4]*v[2]+v[6]*v[21]+v[20]*v[22];
RESULT(v[30] )




EQUATION("LongTermBill")
/*
Shifted moving average of the bill, used as reference value of the bill to pay to compute the attitude to save water
*/

v[0]=V("WaterBill");
v[1]=VL("LongTermBill",1);

if(v[1]==-1)
 {//first period
  END_EQUATION(v[0]);
 } 
v[3]=V("speedLongTermBill");

v[5]=v[1]*v[3]+v[0]*(1-v[3]);

RESULT(v[5] )




EQUATION("DISCARDEDExpectedBill")
/*
Expected bill. It is computed only when the hh receives a new bill, while in other periods it consists of the previous level of expected bill.

On receiving a new bill the hh surveys the neighbours computing their unit water consumption (using the dwelling size as unit) and computing the expected bill for his/her own dwelling size. This value is weighted with the previous expected bill.
 
*/

v[0]=V("WaterBill");
v[1]=VL("WaterBill",1);

if(v[0]==v[1] || v[1]==-1)
 END_EQUATION(VL("ExpectedBill",1));

v[2]=V("Size");

v[3]=v[5]=0;

CYCLE_LINK(curl)
 {
  cur2=LINKTO(curl);
  v[4]=VS(cur2,"Size");
  v[10]=VLS(cur2,"WaterBill",1);
  if(v[10]!=-1)
   {
    v[3]+=v[10];
    v[5]+=1;
   }
 }
if(v[5]==0)
 END_EQUATION(-1);//no neighbour still got a water bill 
v[6]=v[3]/v[5];

v[7]=V("wExpectedBillEffect");
v[8]=v[0]*(1-v[7])+v[7]*v[6];
//INTERACT("EB",v[8]);
RESULT(v[8] )


EQUATION("WaterConsumption")
/*
Amount of water consumed in a day.

It depends on the size of the dwelling (log), the saving attitude, and some random factor
*/

v[0]=V("UnitWater");
v[1]=V("varUnitCon");
v[2]=V("Size");
v[12]=V("minSize");
v[13]=V("maxSize");

v[20]=(v[2]-v[12])/(v[13]-v[12]); //percentage in the scale of size

v[7]=V("Income");
v[17]=V("minIncome");
v[18]=V("maxIncome");
v[21]=(v[7]-v[17])/(v[18]-v[17]); //percentage in the scale of income

v[30]=V("ShareSizeIncome");

v[31]=log(1+v[0]*(v[30]*v[20]+(1-v[30])*v[21]));

v[3]=VL("WaterSaving",1);

v[4]=v[31]*v[3];
v[5]=norm(v[4],v[1]);

RESULT(v[5] )

EQUATION("CumulatedWater")
/*
Cumulated water consumed since last bill
*/

v[0]=V("WaterConsumption");
v[4]=v[0]+VL("CumulatedWater",1);
RESULT(v[4] )



EQUATION("Statistics")
/*
Compute aggregate statistics
*/

v[0]=v[1]=v[2]=v[3]=v[4]=v[5]=v[6]=v[7]=0;

CYCLE(cur, "HH")
 {
  v[0]+=VS(cur,"WaterConsumption");
  v[1]+=VS(cur,"WaterSaving");
  v[2]+=VS(cur,"WaterBill");
  v[3]++;
 }

WRITE("TotalConsumption",v[0]);
WRITE("Revenues",v[2]);
WRITE("AvSaving",v[1]/v[3]);
RESULT(1 )



EQUATION("Init")
/*
Initialization function, executed only once at the start of the simulation and then turned into a parameter to avoid its execution in the following times steps.
*/

v[0]=V("nRows");
v[1]=V("nCols");

p->init_lattice_net( (int)v[0], (int)v[1], "HH", 1);

v[2]=V("minIncome");
v[3]=V("maxIncome");
v[5]=V("minSize");
v[6]=V("maxSize");
v[8]=V("DaysBill");
v[10]=V("UnitWater");
v[13]=V("varUnitCon");
CYCLE(cur, "HH")
 { k=VS_NODEID(cur)-1;
   i=k/(int)v[1];
   j=k-i*(int)v[1];
   WRITES(cur,"row",(double)i+1);
   WRITES(cur,"col",(double)j+1);
   v[4]=UNIFORM(v[2],v[3]);
   WRITES(cur,"Income",v[4]);
   v[7]=UNIFORM(v[5],v[6]);
   WRITES(cur,"Size",v[7]);
   WRITELS(cur,"WaterSaving",v[20]=0.5+RND/2, 1);
   v[9]=rnd_integer(1,v[8]);
   WRITELS(cur,"CounterDays",v[9], 1);
   v[11]=v[7]*v[10]*v[20];
   v[12]=norm(v[11],v[13]);
   v[15]=log(v[12])*(v[8]-v[9]);
   if(isnan(v[15]))
     INTERACTS(cur,"BOH",v[12]);
   WRITELS(cur,"CumulatedWater",v[15], 1);
   
   
    
 }

PARAMETER  
RESULT(1 )








MODELEND




void close_sim(void)
{

}


