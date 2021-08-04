import os
#
# for tbl in os.listdir("./"):
#     if tbl[-4:] == ".tbl":
#         lines = open("./"+tbl)
#         lines2 = open("./2"+tbl, 'w')
#         for line in lines:
#             lines2.write(line[:-1]+"|\n")

lines = open("ORDERLINE.tbl")
d = [0 for col in range(16)]
cnt = [0 for col in range(16)]
for line in lines:
    res = line.split('|')
    if (res[6] > '2007-01-02'):
        d[int(res[3])] += float(res[7])
        cnt[int(res[3])] += 1
print(d)
print(cnt)
