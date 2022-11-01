translator="one_tape"


SUCCESSES_NUM=0
FAILURES_NUM=0

for test in `ls tests/*.tm` ; do
    echo ""
    echo "Testing:" $test
    one_tape=${test}
    ./$translator $test $one_tape
    
    inputs_file="${test%tm}in"
    inputs=()
    mapfile -t inputs < $inputs_file
    inputs+=("")

    echo "on inputs: ${inputs[@]}"

    for input in "${inputs[@]}" ; do
        echo ""
        echo -n "-on input: "
        if [ -z $input ] ; then
            echo "<empty>"
        else
            echo $input
        fi

        echo -n original:  
        original=`./tm_interpreter -q $test "$input"`
        echo $original
        echo -n translated: 
        translated=`./tm_interpreter -q $one_tape "$input"`
        echo $translated
        if [ $translated = $original ] ; then
            echo -e "\e[92mSuccess.\e[39m"
            SUCCESSES_NUM=$(($SUCCESSES_NUM+1))
        else
            echo -e "\e[91mFailure!\e[39m\n"
            FAILURES_NUM=$(($FAILURES_NUM+1))
        fi
    done
done

TOTAL_NUM=$(($FAILURES_NUM+$SUCCESSES_NUM))
echo -e "\n$TOTAL_NUM tests in total, from which"
if (($FAILURES_NUM == 0)); then
   echo -e "\e[92m$SUCCESSES_NUM succeeded\e[39m and"
   echo -e "$FAILURES_NUM failed."
else
    echo -e "$SUCCESSES_NUM succeeded and"
    echo -e "\e[91m$FAILURES_NUM failed\e[39m."
fi

if (($FAILURES_NUM == 0)) ; then
    cowsay "CONGRATULATIONS!"
fi
