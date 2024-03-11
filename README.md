<div align="center">

![header](https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList==0,4,1,1,1,2&height=300&section=header&text=Smart_ventilation_system&fontSize=50)
<br><br><br>

# 🛞 Smart_ventilation_system 🛞
라즈베리파이를 이용한 스마트 환기 시스템  
Smart_ventilation_system with raspberrypi 

<img src="https://img.shields.io/badge/lanaguage-C-8A2BE2&logo=#512BD4"/>
<img src="https://img.shields.io/badge/with-Raspberrypi-8A2BE2"/>
<img src="https://img.shields.io/badge/OS-Linux-8A2BE2&logo=#512CC4"/>
<br><br><br><br><br><br>


# 🖥️ Goal 🖥️
실내 이산화탄소 농도, 실외 미세먼지,내 외부 온도차, 내 외부 습도차, 등을 기반으로<br>
실시간으로 환기를 조절하는 **자동 환기 시스템을** 개발한다.<br>
이산화탄소를 측정할 수 있는 센서와 실외 미세먼지의 실시간 농도 정보를 공급하는 자료처가 필요하며,<br>
3대의 RBP와 이들을 연결하기 위한 리눅스 기반의 시스템 구현이 필요하다.
<br><br><br><br><br><br>


# 📝 Functional analysis 📝
<br>

![그림1](https://github.com/soseongha/Smart-ventilation-system/assets/79681915/218f59c2-f1fd-47b1-addf-65ae596ed413)

<div align="left">
  
**(1) 실내 공기질 자동 측정 기능**<br>
: 특히 실내 이산화탄소 농도 측정 + 온습도 측정<br><br>
**(2) 실외 공기질 자동 측정 기능**<br>
: 미세먼제 농도 측정<br><br>
**(3) 실내외 공기질에 따른 환풍기 개폐 기능**<br>
: 말풍선 내부와 같은 논리판단 후 환풍기의 개폐 여부를 결정<br><br>
**(4) 환풍기 회전 속도 조절 기능**<br>
: 환풍기를 개폐한 뒤에도 공기질 정보가 나쁠 시에 환풍기 속도 조절<br><br>

<br><br><br><br><br><br>


<div align="left">

<div align="center"> 
  
# 🔨 system implementation 🔨
<br>

![그림2](https://github.com/soseongha/Smart-ventilation-system/assets/79681915/2e102af9-7698-4eec-824f-dab024211e34)

<div align="center"> 

<div align="left">

**① RBP1**
- 실내에 설치된 가스센서와 온습도센서로부터 정보를 받아 가공
- last.c
  
**② RBP2**
- 실외의 실시간 미세먼지 농도를 기상청의 open API의 XML 문서 형태로 입력받음
- dust.c
  
**③ RBP3**
- RBP1과 RBP2로부터 정보를 단방향적으로 받아 실내의 공기의 질과 실외의 
공기의 질 정보를 비교 후 최종적으로 환풍기의 on/off 여부를 판단하여 출력 / 환풍기가 on
되어 있다면, RBP1에서 온습도 정보를 연속적인 값으로 받아 회전속도를 조절
- rbp3_fan.c

<div align="left">

<br><br><br><br><br><br>











 


