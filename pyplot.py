OTHER_responseTimeArray = []
with open(r'OTHER-responseTimeArray.txt') as f:
    for line in f:
        fields = line.split()
        rowdata = map(float, fields)
        OTHER_responseTimeArray.extend(rowdata)
        
OTHER_result = sum(OTHER_responseTimeArray)/len(OTHER_responseTimeArray);

print(OTHER_result)


RR_responseTimeArray = []
with open(r'RR-responseTimeArray.txt') as f:
    for line in f:
        fields = line.split()
        rowdata = map(float, fields)
        RR_responseTimeArray.extend(rowdata)

RR_result = sum(RR_responseTimeArray)/len(RR_responseTimeArray);

print(RR_result)


FIFO_responseTimeArray = []
with open(r'FIFO-responseTimeArray.txt') as f:
    for line in f:
        fields = line.split()
        rowdata = map(float, fields)
        FIFO_responseTimeArray.extend(rowdata)

FIFO_result = sum(FIFO_responseTimeArray)/len(FIFO_responseTimeArray);

print(FIFO_result)

