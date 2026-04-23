/* Frame 2 */

position: relative;
width: 800px;
height: 480px;

background: radial-gradient(50% 50% at 50% 50%, #2F009A 1.92%, #2B017E 28.37%, #000000 99.99%);


/* Ellipse 4 */

box-sizing: border-box;

position: absolute;
width: 450px;
height: 450px;
left: calc(50% - 450px/2);
top: calc(50% - 450px/2 + 47px);



/* CURRENT DATA */
/* 电焊机电流参数，对应协议里面的电流，其中Ellipse 5会根据参数进行变动量程为0A-40A */

position: absolute;
width: 113px;
height: 230px;
left: 233px;
top: 162px;



/* 50 */

position: absolute;
left: 0%;
right: 0%;
top: 0%;
bottom: 32.17%;

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 130px;
line-height: 156px;
/* identical to box height */
text-align: center;

background: linear-gradient(180deg, #00F0FF 20.67%, #FF00FF 100%);
-webkit-background-clip: text;
-webkit-text-fill-color: transparent;
background-clip: text;
text-fill-color: transparent;



/* A */

position: absolute;
left: 18.58%;
right: 17.7%;
top: 61.3%;
bottom: 0%;

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 80px;
line-height: 96px;
text-align: center;

background: linear-gradient(180deg, #00F0FF 21%, #FF00FF 100%);
-webkit-background-clip: text;
-webkit-text-fill-color: transparent;
background-clip: text;
text-fill-color: transparent;



/* Ellipse 5 */

position: absolute;
width: 500px;
height: 500px;
left: -83px;
top: -125px;

background: linear-gradient(180deg, #FF2A6D 0%, #FE8C3B 32.69%, #05FF00 92.31%);
border-radius: 10px;


/* AIR PRESSURE */
/* 电焊机气压参数，对应协议里面的气压，且单位可以根据通讯内容改变（PSI/MPa/BAR），其中Ellipse 6会根据参数进行变动量程为0-40 */

position: absolute;
width: 118px;
height: 230px;
left: 453px;
top: 162px;



/* 23 */

position: absolute;
left: 3.39%;
right: 3.39%;
top: 0%;
bottom: 32.17%;

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 130px;
line-height: 156px;
/* identical to box height */
text-align: center;

background: linear-gradient(180deg, #00F0FF 20.67%, #FF00FF 100%);
-webkit-background-clip: text;
-webkit-text-fill-color: transparent;
background-clip: text;
text-fill-color: transparent;



/* Mpa */

position: absolute;
left: 0%;
right: 0%;
top: 61.3%;
bottom: 0%;

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 80px;
line-height: 96px;
text-align: center;

background: linear-gradient(180deg, #00F0FF 36.06%, #FF00FF 100%);
-webkit-background-clip: text;
-webkit-text-fill-color: transparent;
background-clip: text;
text-fill-color: transparent;



/* Ellipse 6 */

position: absolute;
width: 500px;
height: 500px;
left: -303px;
top: -125px;

background: linear-gradient(180deg, #4D00FF 0%, #00F0FF 32.69%, #05FF00 92.31%);
border-radius: 10px;
transform: matrix(-1, 0, 0, 1, 0, 0);


/* BestARC */

position: absolute;
width: 197px;
height: 69px;
left: 301px;
top: 402px;

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 64px;
line-height: 77px;
text-align: center;

background: linear-gradient(180deg, rgba(77, 0, 255, 0.2) 0%, #FF2A6D 100%);
-webkit-background-clip: text;
-webkit-text-fill-color: transparent;
background-clip: text;
text-fill-color: transparent;



/* Component 1 */

position: absolute;
width: 65px;
height: 53px;
left: 722px;
top: 10px;



/* Rectangle 1 */
/* 直接使用素材。蓝牙的工作状态，从协议中可以获取蓝牙是否工作，如果工作则显示，否则不显示该组件 */

position: absolute;
left: 0%;
right: 0%;
top: 0%;
bottom: 0%;

background: linear-gradient(180deg, #4D00FF 0%, #1D97FF 28.85%, #00F0FF 100%);
border-radius: 20px;


/* Icon */

position: absolute;
width: 15.97px;
left: 12.42px;
top: 18.87%;
bottom: 18.87%;

border: 3px solid #F4F4F4;


/* Wifi */

position: absolute;
width: 33px;
height: 35px;
left: 27px;
top: 10px;

transform: rotate(90deg);


/* Icon */

position: absolute;
left: 17.68%;
right: -5.84%;
top: 5.58%;
bottom: 31.93%;

border: 4px solid #F4F4F4;
transform: rotate(90deg);


/* Component 2 */
/* 包含Rectangle 2、STEEL。工作模式的选择，从协议中可以获取当前的工作模式，有2T/4T两种模式，文本内容要根据当前类型动态显示 */

position: absolute;
width: 127px;
height: 77px;
left: calc(50% - 127px/2 - 325.5px);
top: calc(50% - 77px/2 + 187.5px);



/* Rectangle 2 */

position: absolute;
left: 0%;
right: 0%;
top: 3.9%;
bottom: 3.9%;

background: linear-gradient(180deg, #4D00FF 0%, rgba(46, 0, 153, 0.6) 100%);
border-width: 0px 0px 2px 2px;
border-style: solid;
border-color: #4D00FF;
border-radius: 20px;


/* 4T */

position: absolute;
width: 38px;
height: 58px;
left: calc(50% - 38px/2 + 0.5px);
top: calc(50% - 58px/2 + 0.5px);

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 48px;
line-height: 58px;
/* identical to box height */
text-align: center;

color: #F4F4F4;



/* Component 3 */
/* 包含Rectangle 2.1、STEEL。钢材类型的选择，从协议中可以获取当前的钢材类型，有Mesh/Plate/Derusting三种类型，文本内容要根据当前类型动态显示 */

position: absolute;
width: 127px;
height: 77px;
left: calc(50% - 127px/2 - 325.5px);
top: calc(50% - 77px/2 - 0.5px);



/* Rectangle 2.1 */

position: absolute;
left: 0%;
right: 0%;
top: 3.9%;
bottom: 3.9%;

background: linear-gradient(180deg, #4D00FF 0%, rgba(46, 0, 153, 0.6) 100%);
border-width: 0px 0px 2px 2px;
border-style: solid;
border-color: #4D00FF;
border-radius: 20px;


/* STEEL */

position: absolute;
width: 103px;
height: 58px;
left: calc(50% - 103px/2 + 1px);
top: calc(50% - 58px/2 + 0.5px);

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 48px;
line-height: 58px;
/* identical to box height */
text-align: center;

color: #F4F4F4;



/* Component 4 */
/* 包含Rectangle 2.2、220V。工作电压的显示，有240V/220V/110V三种类型，文本内容要根据当前类型动态显示 */

position: absolute;
width: 127px;
height: 77px;
left: calc(50% - 127px/2 - 325.5px);
top: calc(50% - 77px/2 - 188.5px);



/* Rectangle 2.2 */

position: absolute;
left: 0%;
right: 0%;
top: 3.9%;
bottom: 3.9%;

background: linear-gradient(180deg, #4D00FF 0%, rgba(46, 0, 153, 0.6) 100%);
border-width: 0px 0px 2px 2px;
border-style: solid;
border-color: #4D00FF;
border-radius: 20px;


/* 220V */

position: absolute;
width: 82px;
height: 58px;
left: calc(50% - 82px/2 + 0.5px);
top: calc(50% - 58px/2 + 0.5px);

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 48px;
line-height: 58px;
/* identical to box height */
text-align: center;

color: #F4F4F4;



/* Component 5 */
/* 包含Rectangle 3、PILOT arc、5S。显示电焊机的维弧时间，从协议中可以获取当前的维弧时间，有3-15s的范围，文本内容和Rectangle 3计量条要根据当前时间动态显示 */

position: absolute;
width: 63px;
height: 362px;
left: 656px;
top: 103px;



/* Rectangle 3 */

position: absolute;
left: 33.33%;
right: 36.51%;
top: 0%;
bottom: 17.4%;

background: linear-gradient(180deg, #00F0FF 0%, #05FF00 100%);
border-radius: 10px;


/* PILOT arc */

position: absolute;
left: 0%;
right: 0%;
top: 93.92%;
bottom: 0%;

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 18px;
line-height: 22px;
/* identical to box height */
text-align: center;

color: #F4F4F4;



/* 5S */

position: absolute;
left: 28.57%;
right: 28.57%;
top: 84.25%;
bottom: 5.25%;

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 32px;
line-height: 38px;
text-align: center;

color: #F4F4F4;



/* Component 6 */
/* 包含Rectangle 4、POST aIR、9S。显示电焊机的后气时间，从协议中可以获取当前的后气时间，有3-15s的范围，文本内容和Rectangle 4计量条要根据当前时间动态显示 */

position: absolute;
width: 56px;
height: 362px;
left: 732px;
top: 103px;



/* Rectangle 4 */

position: absolute;
left: 32.14%;
right: 33.93%;
top: 0%;
bottom: 17.4%;

background: linear-gradient(180deg, #00F0FF 0%, #05FF00 100%);
border-radius: 10px;


/* POST aIR */

position: absolute;
left: 0%;
right: 0%;
top: 93.92%;
bottom: 0%;

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 18px;
line-height: 22px;
/* identical to box height */
text-align: center;

color: #F4F4F4;



/* 9S */

position: absolute;
left: 25%;
right: 25%;
top: 84.25%;
bottom: 5.25%;

font-family: 'BBH Bogle';
font-style: normal;
font-weight: 400;
font-size: 32px;
line-height: 38px;
text-align: center;

color: #F4F4F4;

