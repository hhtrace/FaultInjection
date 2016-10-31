#positions=PT_F32 PT_F33 PT_F34 PT_F35 PT_F36 PT_F37 PT_F38 PT_F39 PT_F40
#types=1_bit_flip 2_bit_flip 1_bit_0 2_bit_0 1_bit_1 2_bit_1

RESULT_DIR=RESULT
mkdir -p $RESULT_DIR
#for POS in PT_F40
for (( NUM = 400; NUM <= 1000; NUM += 50 ))
do
    mkdir -p $RESULT_DIR/$NUM
    echo "positions        types         sigalrm   sigsegv   sigill" >> $RESULT_DIR/$NUM/result.log
    #for POS in PT_F32 PT_F33 PT_F34 PT_F35 PT_F36 PT_F37 PT_F38 PT_F39 PT_F40
    for POS in PT_F40
    do
        mkdir -p $RESULT_DIR/$NUM/$POS
        for TYPE in 1_bit_flip 2_bit_flip 1_bit_0 2_bit_0 1_bit_1 2_bit_1 8_low_1 8_low_0 8_low_error plus_1 plus_2 plus_3 plus_4 plus_5
        do
            echo -e "$POS $TYPE $NUM"
            for (( i = 1; i <= $NUM; i++))
            do
                echo -e "$POS-$TYPE-$NUM Test Num : $i"
                echo -e "$POS-$TYPE-$NUM Test Num : $i" >> $RESULT_DIR/$NUM/$POS/$POS-$TYPE-$NUM.log
	            #../bin/hello &
                ../bin/injector --step 100000 --mode microsecond --pos $POS --type $TYPE -e ../bin/hello >> $RESULT_DIR/$NUM/$POS/$POS-$TYPE-$NUM.log 
            done
            python ./readlog.py -p $POS -t $TYPE -n $NUM >> $RESULT_DIR/$NUM/result.log
        done
    done
done
