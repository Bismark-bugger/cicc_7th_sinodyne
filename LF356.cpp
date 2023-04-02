//主要函数
/*
*板载资源
1路高压器件电压源-HDPS 1路参数测量单元  1路电压电流源-PMU
音频电压表  音频电压源-AVM  4路偏置电源  2路时间测量单元

*电压控制
SET_VSx  10~13  (电压值,单位) 设置第x路偏压源电压,1~4
CLEAR_VS  14  () 清电压源
SET_DPS_NEG  7  (负电源电压值,单位,箝位电流值,箝位单位) 设置HDPS正电压
SET_DPS_POS  4  (正电源电压值,单位,箝位电流值,箝位单位) 设置HDPS负电压  -类似给激励？
SET_AS  15  (音频电压值,单位,频率,频率单位) 设置音频电压源交流输出
SET_PMU  21-51页,图形文件  (模式,施加值,单位,箝位值,单位) 设置PMU

*测量 
DVM_MEASURE  27  (DVM通道编号,量程,单位,测量延迟时间) 直接电压测量
DVM_DIFF_MEASURE  29  (DVM通道编号,量程,单位,间隔时间) 间隔时间直接电压测量,例子？？？
DPS_MEASURE_POS  5  (测量延迟时间)  测量HDPS正电源电流
DPS_MEASURE_NEG  8  (测量延迟时间)  测量HDPS负电源电流
SET_AVM_PATH  22  (通路,带通) 设置音频电压测量通路,离谱的例子
AVM_MEASURE  23  (AVM通道编号,量程,单位,测量延迟时间) 测量音频电压
TIME_MEASURE_EXECUTE  25  (测量时间) 时间测量，连用？

*继电器操作
SET_RELAY  32  (继电器号) 闭合指定继电器,打开其他
CLOSE_RELAY  33  (继电器号) 闭合指定继电器,其他不变
CLEAR_RELAY  34  (继电器号) 打开指定继电器

*测试结果操作
SHOW_RESULT  35  (测试项目名称,测试值,单位,上限,下限)  显示测试结果
BIN  37  (分箱号)  用于测试完成的分箱处理,一项一个,没懂啥用
*/

//LF356测试程序

#include "StdAfx.h"
#include "testdef.h"
#include "data.h"
#include "math.h"     //头文件生成自带

bool flag=true;
double caldata;

void PASCAL LF356()
{
  double M_V, M_V1, V1, V2, I1, I2, IB1, IB2, Vos;

  //校准(测量-1nA电流所对应的积分电压)
  if(flag)
  {
    SET_VS3(1,V);  
    SET_RELAY("19");   
    Delay(20);
    CLOSE_RELAY("21");   
    Delay(20);
    caldata = DVM_DIFF_MEASURE(6,15,V,150);   
    CLEAR_VS();
    flag = false;
  }

  //电源电流
  SET_DPS_NEG(-15,V,20,MA);
  SET_DPS_POS(15,V,20,MA);
  SET_VS4(0,V);
  SET_RELAY("4,9");
  I1 = DPS_MEASURE_POS(10)*1000;
  I2 = DPS_MEASURE_NEG(10)*1000;
  if(!SHOW_RESULT("IDD",I1,"mA",10,No_LoLimit))
    BIN(1);
  if(!SHOW_RESULT("ISS",I2,"mA",No_UpLimit,-10))
    BIN(1);

  //Input Bias Current(IB)  输入偏置电流
  //当输出维持在规定的电平时，两个输入端流进电流的平均值
  SET_RELAY("4,7,9,13");
  Delay(20);
  CLOSE_RELAY("21");    
  Delay(20);   
  V2 = DVM_DIFF_MEASURE(6,5,V,300)/2;
  IB2 = -V2/caldata*1000;
  if(!SHOW_RESULT("IB(-)",IB2,"pA",200,-200))
    BIN(2);
  SET_RELAY("4,9,13");
  Delay(20);
  CLOSE_RELAY("21");
  Delay(20);
  V1 = DVM_DIFF_MEASURE(6,5,V,300)/2;
  IB1 = -V1/caldata*1000;
  if(!SHOW_RESULT("IB(+)",IB1,"pA",200,-200))
    BIN(2);

  //Input Offset Current(IOS)   输入失调电流
  //当输出维持在规定的电平时，两个输入端流进电流的差值
  I1 = IB1-IB2;
  if(!SHOW_RESULT("Ios",I1,"pA",50,-50))
    BIN(3);

  //Input Offset Voltage(Vos)   输入失调电压
  //在运放开环使用时,加载在两个输入端之间的直流电压使得放大器直流输出电压为0
  SET_RELAY("4,9");
  M_V = DVM_MEASURE(4,5,V,10)/401;
  Vos = M_V*1000;
  if(!SHOW_RESULT("Vos",Vos,"mV",10,-10))
    BIN(4);

  //Supply Voltage Rejection Ratio(PSRR)   电源电压抑制比
  //把电源的输入与输出看作独立的信号源,输入与输出的纹波比值即是PSRR,运放对电源上纹波或者噪声的抵抗能力
  SET_DPS_NEG(-5,V,20,MA);
  SET_DPS_POS(5,V,20,MA);
  V1 = DVM_MEASURE(4,5,V,10)/401;
  V2 = fabs(M_V-V1);
  M_V = 20*log10(20/V2);
  if(!SHOW_RESULT("PSRR",M_V,"dB",No_UpLimit,80))
    BIN(5);

  //Output Voltage Swing(VO)   输出电压摆幅
  //在给定电源电压和负载情况下,输出能够达到的最大电压范围
  SET_DPS_NEG(-15,V,20,MA);
  SET_DPS_POS(15,V,20,MA);
  SET_PMU(FVMI,0,V,50,MA);
  SET_VS4(-15,V);
  CLOSE_RELAY("24");
  V1 = DVM_MEASURE(5,15,V,10);
  SET_VS4(15,V);
  V2 = DVM_MEASURE(5,15,V,10);
  if(!SHOW_RESULT("+VO(RL=10K)",V1,"V",No_UpLimit,12))
    BIN(6);
  if(!SHOW_RESULT("-VO(RL=10K)",V2,"V",-12,No_LoLimit));
    BIN(6);
  CLEAR_RELAY("24");
  CLOSE_RELAY("25");
  V2 = DVM_MEASURE(5,15,V,10);
  SET_VS4(-15,V);
  V1 = DVM_MEASURE(5,15,V,10);
  if(!SHOW_RESULT("+VO(RL=2K)",V1,"V",No_UpLimit,10))
    BIN(6);
  if(!SHOW_RESULT("-VO(RL=2K)",V2,"V",-10,No_LoLimit));
    BIN(6);

  //Avo   开环电压增益
  //运放本身具备的输出电压与两个输入端差压的比值,单位dB
  SET_VS4(-10,V);
  V1 = DVM_MEASURE(4,5,V,10)/401;
  Delay(10);
  SET_VS4(10,V);
  V2 = DVM_MEASURE(4,5,V,10)/401;
  M_V1 = fabs(V1-V2);
  if(M_V1==0.0)
    M_V1 = 0.000004;
  M_V = 20/(M_V1*1000);
  if(!SHOW_RESULT("Avo",M_V,"V/mV",No_UpLimit,25))
    BIN(7);
  CLEAR_RELAY("25");
  CLEAR_VS();

  //Common_Mode Rejection Ratio(CMRR)   共模抑制比
  //差模电压增益与共模电压增益的比值
  SET_DPS_POS(5,V,20,MA);
  SET_DPS_NEG(-25,V,20,MA);
  SET_VS4(10,V);
  V1 = DVM_MEASURE(4,5,V,10)/401;
  SET_VS4(0,V);
  SET_DPS_NEG(-5,V,20,MA);
  SET_DPS_POS(25,V,20,MA);
  SET_VS4(-10,V);
  V2 = DVM_MEASURE(4,5,V,10)/401;
  CLEAR_VS();
  M_V = fabs(V2-V1);
  if(M_V<0.000001)
    M_V = 0.000001;
  M_V = 20/M_V;
  M_V = 20*log10(M_V);
  if(!SHOW_RESULT("CMRR",M_V,"dB",No_UpLimit,80))
    BIN(8);
  
  //Input Common_Mode Voltage Range(Vcm)   共模电压输入范围
  //运放两输入端与地间能加的共模电压范围
  M_V = fabs(V1-Vos/1000);
  if(M_V<0.000001)
  M_V = 0.000001;
  M_V = 10/M_V;
  M_V = 20*log10(M_V);
  if(!SHOW_RESULT("Vcm(+)",M_V,"dB",No_UpLimit,74))
    BIN(9);
  M_V = fabs(V2-Vos/1000);
  if(M_V<0.000001)
  M_V = 0.000001;
  M_V = 10/M_V;
  M_V = 20*log10(M_V);
  if(!SHOW_RESULT("Vcm(-)",M_V,"dB",No_UpLimit,74))
    BIN(9);

  //Gain-Bandwidth Product   增益带宽积
  //运放开环增益/频率图中,指定频率处,开环增益与该指定频率的乘积
  SET_DPS_POS(15,V,20,MA);
  SET_DPS_NEG(-15,V,20,MA);
  SET_VS4(0,V);
  SET_RELAY("4,9,14");
  SET_AS(1,V,100000);
  SET_AVM_PATH(LPPASS,BPPASS);
  V1 = AVM_MEASURE(1,4,V,100);
  V2 = AVM_MEASURE(2,2,V,100);
  M_V = 40.1*V2/V1;
  if(!SHOW_RESULT("GBW",M_V,"MHz",No_UpLimit,No_LoLimit))
    BIN(10);

  //SLEW RATE   压摆率
  //闭环放大器输出电压变化的最快速率
  SET_RELAY("6,27");
  TIME_MEASURE_EXECUTE(100)*1000000;
  M_V = 8/M_V;
  if(!SHOW_RESULT("SR",M_V,"V/uS",No_UpLimit,No_LoLimit))
    BIN(11);
}