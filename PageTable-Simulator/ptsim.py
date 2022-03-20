import sys, getopt
from sys import stdin

def main(argv):
    clock = int(sys.argv[2])
    f = open(sys.argv[1],"r")
    base = f.readline()                  #reads table header for table info
    base = base.split()
    end = int(base[2])
    length = 0
    while end != 1:                      #finds length of page offset
        end = end/2
        length += 1
    table = []
    for line in f:
        line = line.split()              #splits each remaining entry
        if (line != []):                 #checks for blank lines
            for i in range(len(line)):
                line[i] = int(line[i])
            table.append(line)           #stores in table
    index = 0                            #sets starting index for clock algorithm
    while (table[index][0]==0):          #find first valid row in table
        index += 1
    for line in stdin:
        if (line[1] == 'x'):             #checks for hex input
            input_val = int(line,16)
        else:
            input_val = int(line)
        virtual = input_val >> length    #shifts over to leave just the table row in the virtual address
        if(table[virtual][0]):           #check if row is valid
            physical = table[virtual][2] #convert address from virtual to physical
            output_val = input_val - (virtual << length)
            output_val = output_val + (physical << length)
            print(hex(output_val))
            if (clock):                  #set as recently used if using clock algorithm
                table[virtual][3] = 1
        else:
            if(table[virtual][1]==0):    #checks the permission bit
                print("SEGFAULT")
            elif (clock):                #utilizes the clock algorithim
                print("PAGEFAULT",end=' ')
                while (table[virtual][0]==0):
                    if (index == len(table)):
                        index = 0
                    if (table[index][0]):
                        if(table[index][3]):
                            table[index][3] = 0
                            index += 1
                        else:
                            table[index][0] = 0
                            table[virtual][0] = 1
                            temp = table[virtual][2]
                            table[virtual][2] = table[index][2]
                            table[index][2] = temp
                            table[virtual][3] = 1
                    else:
                        index += 1
                physical = table[virtual][2]
                output_val = input_val - (virtual << length)
                output_val = output_val + (physical << length)
                print(hex(output_val))
            else:
                print("DISK")
    return 0

if __name__ == "__main__":
   main(sys.argv)
