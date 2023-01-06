A1_releaseDiff = []
with open(r'A1-ReleaseTimeDiff.txt') as f:
    for line in f:
        fields = line.split()
        rowdata = map(float, fields)
        A1_releaseDiff.extend(rowdata)
        


A2_releaseDiff = []
with open(r'A2-ReleaseTimeDiff.txt') as f:
    for line in f:
        fields = line.split()
        rowdata = map(float, fields)
        A2_releaseDiff.extend(rowdata)

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

X_values = ["1","2","3","4","5","6","7","8","9","10"]
Y_values_1 = A1_releaseDiff
Y_values_2 = A2_releaseDiff
Y_values_3 = [0,0,0,0,0,0,0,0,0,0]

fig, ax1 = plt.subplots(figsize=(12,9))
title = ('Comparison on difference of actual and expected release time among 3 techniques')
plt.title(title,fontsize=20)
plt.grid(axis='y',color='grey',linestyle='--',lw=0.5,alpha=0.5)
plt.tick_params(axis='both',labelsize=14)
plot1 = ax1.plot(X_values,Y_values_1,'b',label='Alternative 1')
ax1.set_xlabel('Number of execution periods', fontsize = 18)
ax1.set_ylabel('Difference (Actual - Expected) ns', fontsize = 18)
ax1.set_ylim(0,8e7)
for tl in ax1.get_yticklabels():
    tl.set_color('b')  
    
ax2 = ax1.twinx()
plot2 = ax2.plot(X_values,Y_values_2,'g',label='Alternative 2')
ax2.set_ylabel('Difference (Actual - Expected) ns',fontsize=18)
ax2.set_ylim(0,3e7)
ax2.tick_params(axis='y',labelsize=14)
for tl in ax2.get_yticklabels():
    tl.set_color('g')                    
# ax2.set_xlim(1966,2014.15)

ax3 = ax1.twinx()
plot3 = ax3.plot(X_values,Y_values_3,'r',label='Correct Implementation')
ax3.set_ylim(0,3e7)
ax3.tick_params(axis='y',labelsize=14)
for tl in ax3.get_yticklabels():
    tl.set_color('r')                    
# ax2.set_xlim(1966,2014.15)

lines = plot1 + plot2 + plot3   
ax1.legend(lines,[l.get_label() for l in lines])    
ax1.set_yticks(np.linspace(ax1.get_ybound()[0],ax1.get_ybound()[1],9)) 
ax2.set_yticks(np.linspace(ax2.get_ybound()[0],ax2.get_ybound()[1],9)) 
ax3.set_yticks(np.linspace(ax3.get_ybound()[0],ax3.get_ybound()[1],1)) 
for ax in [ax1,ax2,ax3]:
    ax.spines['top'].set_visible(False)
    ax.spines['bottom'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_visible(False)                       
fig.text(0.1,0.02,'Plotted for CESE4025 Assignment 3 by Group 67',fontsize=10)
plt.savefig("Comparisons2.png")