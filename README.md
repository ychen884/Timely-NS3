# Timely-NS3

This is a Timely implementation based on TCP congestion control module in ns3

## Build

1. NS3 version 3.33
Please follow the instructions below to build ns3-3.33
https://www.nsnam.org/releases/ns-3-33/documentation/
https://www.nsnam.org/wiki/Installation#Linux


2. I would also share my virtual machine image, you can get one from here [Coming soon]

3. Scenerio: a simple incast topology with one sink and N source, with limited egress bandwidth.
Please check the comment in test.cc for more detailed topology description.


## Run

1. Run basic program

Example:
running  10 nodes sending into one sink node with a egress bandwidth 25Mbps
/waf --run "scratch/test --congestion=TCPCCTIMELY --incast=10"
The command below runs the same parameter as the default ones, but explicitly set the options. Check this to learn how to use these cmd options.
./waf --run "scratch/test --congestion=TCPCCTIMELY --incast=10 --Alpha=0.6 --AI=1.0 --MD=0.05 --initial_rate=5 --Hth=4500 --Lth=500 --HAI=5"

CMD argument options with default value inside bracket[]:

	--congestion:	Protocol to use: TCPCCTIMELY, TcpVegas, TcpDctcp [TCPCCTIMELY]; Notice all other supported protocol can also work, but requiring one to modify the source code becuase I add call back to trace RTTs
	--h:         	Showing man [false]; --h=true shows the man page
	--bandwitdh: 	egress bandwidth [25Mbps]
	--incast:    	incast num [15]
	--incast:    	active incast [15]
	--Alpha:     	Alpha [0.6]; EWMA weight, the larger weight, we put more value to historical data. Range: [0,1]
	--AI:        	AI [1]; Additive increasing parameter
	--MD:        	MD [0.05]; Multiplicative decreasing parameter
	--initial_rate:  initial_rate [5]: initial sending rate
	--Hth:       	Hth [4500]: Higher threshold
	--Lth:       	Lth [500]: Lower threshold
	--HAI:       	HAI counter [5]: HAI counter, number of completion events that triggers HAI mode

2. Export txt stat file and show the graphs:

./waf --run "scratch/test --congestion=TCPCCTIMELY --incast=10" >> output_25_10.txt (you can set any output file, and any running cmd options if you want)
Make sure you have python 3.8 with matplotlib installed
Then run the python script main.py, make sure the output file is in the same directory
I have provided the main.py in /Draw and my txt file to generate my graphs in the /Graph （LF be replaced by CRLF becuase of github）

