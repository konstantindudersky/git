values = []
for line in open("C:\\ExportCSV\\Kiln_Feed_Hour.txt").readlines():
    print(line)
    values.append(int(line.split(','), delimiter='.')[1])

print(values)