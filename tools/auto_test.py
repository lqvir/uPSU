from multiprocessing.connection import wait
import os
import pwd
import re
import threading
import _thread
import subprocess
import random
import string
import json
import time
import signal
json_name_list = []
data_set_list=[]
query_set_list=[]
def check_ans(db_name,query_name,union_name):
    db = []
    query = []
    union = []
    with open(db_name,"r") as db_read:
        db = db_read.readlines()
    with open(query_name,"r") as query_read:
        query = query_read.readlines()
    with open(union_name,"r") as union_read:
        union = union_read.readlines()
    db_set = set(db)
    query_set =set(query)
    union_set = set(union)

    ins = db_set.intersection(query_set)
    un = db_set.union(query_set)
    err_set = union_set.intersection(db_set)
    que_set = union_set.intersection(query_set)
    err_ins = union_set.intersection(ins)
    assert len(err_set)==0,"ot get item in dbset"
    assert len(err_ins)==0,"ot get item in intersection"
    assert len(ins) + len(union_set) == len(query_set ), "ot forget item in query"
    assert len(que_set) == len(union_set),"ot get item not in query"
    print("success")
def sender_func(id,t):
    send_cmd = [sender_c[t],"-q "+query, "-a 127.0.0.1","--port 60000",thread_c[t]]
    print(send_cmd)
    
    outfile = subprocess.Popen(send_cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    outfile.wait(200)
    # time.sleep(50)
    # outfile.send_signal(signal.SIGINT)
    with open("table10","a+") as fp:
        for i in outfile.stdout.readlines():
            fp.write(i.decode())
        fp.write("===================================one call finish =========================================\n\n\n")
def receiver_func(id,t):
    recv_cmd = [recv_c[t],"-d "+db, "--port 60000","-p "+param,thread_c[t],item_len]
    print(recv_cmd)
    
   
    outfile = subprocess.Popen(recv_cmd,stdout=subprocess.PIPE)
    # time.sleep(50)
    # outfile.send_signal(signal.SIGINT)
    for i in range(200):
        if outfile.poll()!= None:
            break
        else:
            time.sleep(1)
        if(i == 99):
            outfile.send_signal(signal.SIGINT)
            break
    

    with open("table10","a+") as fp:
        for i in outfile.stdout.readlines():
            fp.write(i.decode())
        fp.write("===================================one call finish =========================================\n\n\n")
class send_thread(threading.Thread):
    def __init__(self, threadID, name,t):
        threading.Thread.__init__(self)
        self.name = name
        self.threadID = threadID
        self.t = t
    def run(self):
        print ("thread start "+self.name)
        sender_func(self.threadID,self.t)
        print ("thread finish "+self.name)
class recv_thread(threading.Thread):
    def __init__(self, threadID, name,t):
        threading.Thread.__init__(self)
        self.name = name
        self.threadID = threadID
        self.t = t
    def run(self):
        print ("thread start"+self.name)
        receiver_func(self.threadID,self.t)
        print ("thread finish"+self.name)


def PSU_work():
    senders = []
    receivers = []
    senders.append(send_thread(0, "sender"))
    receivers.append(recv_thread(0, "receiver"))
 


def prepare_data(sender_sz,recv_sz,int_sz,item_bc):
    data_set_name='db.csv'
    query_set_name='query.csv'
    
    label_bc = 0
    
    sender_list = []

    letters = string.ascii_lowercase + string.ascii_uppercase
    while len(sender_list) < sender_sz:
        item = ''.join(random.choice(letters) for i in range(item_bc))
        label = ''.join(random.choice(letters) for i in range(label_bc))
        sender_list.append((item, label))
    print('Done creating sender\'s set')

    recv_set = set()
    while len(recv_set) < min(int_sz, recv_sz):
        item = random.choice(sender_list)[0]
        recv_set.add(item)

    while len(recv_set) < recv_sz:
        item = ''.join(random.choice(letters) for i in range(item_bc))
        recv_set.add(item)
    print('Done creating receiver\'s set')

    with open(data_set_name, "w") as sender_file:
        for (item, label) in sender_list:
            sender_file.write(item + (("," + label) if label_bc != 0 else '') + '\n')
    print('Wrote sender\'s set   '+data_set_name)

    with open(query_set_name, "w") as recv_file:
        for item in recv_set:
            recv_file.write(item + '\n')
    print('Wrote receiver\'s set    '+query_set_name)
def prepare_json():
    PSU_pram = {
              "table_params": {
                "hash_func_count": 3,
                "table_size": 1638,
                "max_items_per_bin": 128
                 },
                "item_params": {
                    "felts_per_item": 5
                },
                "query_params": {
                    "ps_low_degree": 44,
                    "query_powers": [ 1, 3, 11, 18, 45, 225 ]
                },
                "seal_params": {
                    "plain_modulus_bits": 22,
                    "poly_modulus_degree": 8192,
                    "coeff_modulus_bits": [ 56, 56, 56, 50 ]
                }
                }
    
    json_name = "out.json"
    with open(json_name,"w") as f:
         json.dump(PSU_pram,f)
    json_name_list.append(json_name)
def work_fun(table):
    
    for i in table:
        start = time.time()

        senders= send_thread(0, "sender",t = i)
        receivers= recv_thread(1, "receiver",t=i)
        receivers.start()
        senders.start()

        receivers.join()
        senders.join()
        check_ans(db,query,union)
        end = time.time()

        with open("python-out.txt","a+") as fp:
            fp.write(str(end-start)+'\n')
        print('====================================================================')
def PCST_fun(table):
    time_list = []
    for i in table:
        start = time.time()
        senders= send_thread(0, "sender",t = i)
        receivers= recv_thread(1, "receiver",t=i)
        receivers.start()
        
        senders.start()
      
        receivers.join()
        senders.join()
        end = time.time()
        time_list.append(end-start)
    with open("python-out.txt","a+") as fp:
        fp.write(str(time_list)+'\n')
    print('====================================================================')
def network10G():
    cmd_t = ["tc","qdisc", "change", "dev","lo",  "root", "handle", "1:0" ,"tbf" ,"lat" ,"10ms" ,"rate" ,"10Gbit" ,"burst" ,"1G"]
    print(cmd_t)
    with open("python-out.txt","a+") as fp:
        fp.write(str(cmd_t)+'\n')
    subprocess.run(cmd_t)
    cmd_t1 = [ "tc", "qdisc", "change", "dev","lo", "parent", "1:1" ,"handle" ,"10:" ,"netem" ,"delay" ,"0.1msec"]
    print(cmd_t1)
    with open("python-out.txt","a+") as fp:
        fp.write(str(cmd_t1)+'\n')
    subprocess.run(cmd_t1)
def network100M():
    cmd_t = ["tc","qdisc", "change", "dev","lo",  "root", "handle", "1:0" ,"tbf" ,"lat" ,"10ms" ,"rate" ,"100Mbit" ,"burst" ,"10M"]
    print(cmd_t)
    with open("python-out.txt","a+") as fp:
        fp.write(str(cmd_t)+'\n')
    subprocess.run(cmd_t)
    cmd_t1 = [ "tc", "qdisc", "change", "dev","lo", "parent", "1:1" ,"handle" ,"10:" ,"netem" ,"delay" ,"40msec"]
    print(cmd_t1)
    with open("python-out.txt","a+") as fp:
        fp.write(str(cmd_t1)+'\n')
    subprocess.run(cmd_t1)
def network10M():
    cmd_t = ["tc","qdisc", "change", "dev","lo",  "root", "handle", "1:0" ,"tbf" ,"lat" ,"10ms" ,"rate" ,"10Mbit" ,"burst" ,"1M"]
    print(cmd_t)
    with open("python-out.txt","a+") as fp:
        fp.write(str(cmd_t)+'\n')
    subprocess.run(cmd_t)
    cmd_t1 = [ "tc", "qdisc", "change", "dev","lo", "parent", "1:1" ,"handle" ,"10:" ,"netem" ,"delay" ,"40msec"]
    print(cmd_t1)
    with open("python-out.txt","a+") as fp:
        fp.write(str(cmd_t1)+'\n')
    subprocess.run(cmd_t1)
if __name__ =="__main__":
    
    db = "db.csv"
    query = "query.csv"
    param = '16M-1024.json'
    union = "union.csv"
    
    thread_c = ["-t 1","-t 2","-t 4","-t 1","-t 4","-t 8"]
    sender_c = ["./sender1","./sender4","./sender8"]
    recv_c = ["./receiver1","./receiver4","./receiver8"]
    sender_c = ["./sender_cli1","./sender_cli2","./sender_cli4","./sender_cli1_M","./sender_cli4","./sender_cli8"]
    recv_c = ["./receiver_cli1","./receiver_cli2","./receiver_cli4","./receiver_cli1_M","./receiver_cli4","./receiver_cli8"]

    table = [1]
    item_bc = 16
    item_len = "--len "+str(item_bc)
    network100M()
 	
    # prepare_data(pow(2,18),pow(2,10),256,item_bc)
    # work_fun(table)

    # prepare_data(pow(2,19),pow(2,10),256,item_bc)
    # work_fun(table)

    # prepare_data(pow(2,20),pow(2,10),256,item_bc)
    # work_fun(table)
    # work_fun(table)
    
   # prepare_data(pow(2,22),pow(2,10),256,item_bc)
    #work_fun(table)

    param = '16M-2048.json'

    prepare_data(pow(2,18),pow(2,11),256,item_bc)
    work_fun(table)
    
    prepare_data(pow(2,19),pow(2,11),256,item_bc)
    work_fun(table)

    prepare_data(pow(2,20),pow(2,11),256,item_bc)
    work_fun(table) 
   # prepare_data(pow(2,22),pow(2,11),256,item_bc)

   # work_fun(table)
    # network100M()
    # PCST_fun(table)

    # network10M()
    # PCST_fun(table)
    #prepare_data(pow(2,18),pow(2,11),256)
    #work_fun()
    
 
"""
    cmd_t = ["tc","qdisc", "change", "dev","lo",  "root", "handle", "1:0" ,"tbf" ,"lat" ,"10ms" ,"rate" ,"10Gbit" ,"burst" ,"1G"]
    print(cmd_t)
    subprocess.run(cmd_t)
    cmd_t1 = [ "tc", "qdisc", "change", "dev","lo", "parent", "1:1" ,"handle" ,"10:" ,"netem" ,"delay" ,"0.1msec"]
    print(cmd_t1)
    subprocess.run(cmd_t1)
    
    

    work_fun()


    cmd_t = ["tc","qdisc", "change", "dev","lo",  "root", "handle", "1:0" ,"tbf"  ,"rate" ,"10Mbit" ,"burst" ,"1M","lat" ,"10ms"]
    print(cmd_t)
    subprocess.run(cmd_t)
    work_fun()
"""
# with open("a","r") as a_read:
#     a = a_read.readlines()
# with open("b","r") as b_read:
#     b = b_read.readlines()
# for i in range(len(a)):
#     if(a[i]==b[i] and i%2 ==0):
#         print(i,end=' ')
