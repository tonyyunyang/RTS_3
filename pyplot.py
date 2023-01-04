OTHER_responseTimeArray = []
with open(r'OTHER-responseTimeArray.txt') as f:
    for line in f:
        fields = line.split()
        rowdata = map(float, fields)
        OTHER_responseTimeArray.extend(rowdata)
        
OTHER_result = sum(OTHER_responseTimeArray)/len(OTHER_responseTimeArray)
OTHER_misses = 5;
print(OTHER_result)


RR_responseTimeArray = []
with open(r'RR-responseTimeArray.txt') as f:
    for line in f:
        fields = line.split()
        rowdata = map(float, fields)
        RR_responseTimeArray.extend(rowdata)

RR_result = sum(RR_responseTimeArray)/len(RR_responseTimeArray)
RR_misses = 3;
print(RR_result)


FIFO_responseTimeArray = []
with open(r'FIFO-responseTimeArray.txt') as f:
    for line in f:
        fields = line.split()
        rowdata = map(float, fields)
        FIFO_responseTimeArray.extend(rowdata)

FIFO_result = sum(FIFO_responseTimeArray)/len(FIFO_responseTimeArray)
FIFO_misses = 3;
print(FIFO_result)

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

X_values = ["FIFO","RR", "OTHER"]
Y_values_1 = [FIFO_result, RR_result, OTHER_result]
Y_values_2 = [FIFO_misses, RR_misses, OTHER_misses]

fig, ax1 = plt.subplots(figsize=(12,9))
title = ('Comparison on response time & deadline misses between FIFO, RR and OTHER')
plt.title(title,fontsize=20)
plt.grid(axis='y',color='grey',linestyle='--',lw=0.5,alpha=0.5)
plt.tick_params(axis='both',labelsize=14)
plot1 = ax1.plot(X_values,Y_values_1,'b',label='Value of response time')
ax1.set_ylabel('Response Time', fontsize = 18)
# ax1.set_ylim(0,240)
for tl in ax1.get_yticklabels():
    tl.set_color('b')  
    
ax2 = ax1.twinx()
plot2 = ax2.plot(X_values,Y_values_2,'g',label='No. of deadline misees')
ax2.set_ylabel('Deadline misses cases',fontsize=18)
ax2.set_ylim(2,5)
ax2.tick_params(axis='y',labelsize=14)
for tl in ax2.get_yticklabels():
    tl.set_color('g')                    
# ax2.set_xlim(1966,2014.15)

lines = plot1 + plot2           
ax1.legend(lines,[l.get_label() for l in lines])    
ax1.set_yticks(np.linspace(ax1.get_ybound()[0],ax1.get_ybound()[1],9)) 
ax2.set_yticks(np.linspace(ax2.get_ybound()[0],ax2.get_ybound()[1],4)) 
for ax in [ax1,ax2]:
    ax.spines['top'].set_visible(False)
    ax.spines['bottom'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_visible(False)                       
fig.text(0.1,0.02,'Plotted for CESE4025 Assignment 3 by Group 67',fontsize=10)
plt.savefig("Comparisons.png")