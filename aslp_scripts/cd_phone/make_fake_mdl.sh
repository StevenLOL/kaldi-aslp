#!/bin/bash

# Created on 2016-03-04
# Author: Binbin Zhang

if [ $# != 2 ]; then
    echo "Make mono state fake model"
    echo "Usage: $0 num_state out_mdl_file"
    echo "eg: $0 215 fake.mdl"
    exit -1;
fi

num_state=$1
mdl_file=$2

{
echo "<TransitionModel>"
echo "<Topology>"
echo "<TopologyEntry>"
echo "<ForPhones>"
for((i=2; i<=$num_state; i++)); do 
    echo -n "$i "
done
echo ""
echo "</ForPhones>"
echo "<State> 0 <PdfClass> 0 <Transition> 0 0.75 <Transition> 1 0.25 </State>"
echo "<State> 1 </State>"
echo "</TopologyEntry>"

# for SIL(sil) phone, it's self loop prob is bigger
echo "<TopologyEntry>"
echo "<ForPhones>"
echo "1 " # Here should be sil phone id
echo "</ForPhones>"
echo "<State> 0 <PdfClass> 0 <Transition> 0 0.75 <Transition> 1 0.25 </State>"
echo "<State> 1 </State>"
echo "</TopologyEntry>" 
echo "</Topology>"

echo "<Triples> $num_state"
for((i=1; i<=$num_state; i++)); do 
    echo "$i 0 $[i-1]"
done
echo "</Triples>"
echo "<LogProbs>"
echo -n "[ 0 "
for ((i=1; i<=$num_state; i++)); do
    echo -n "-0.693147 -0.693147 "
done
echo "]"
echo "</LogProbs>"
echo "</TransitionModel>"
echo "<DIMENSION> 39 <NUMPDFS> $num_state"
for ((i=1; i<=$num_state; i++)); do
    echo "<DiagGMM> 
<GCONSTS>  [ -36.75754 ]
<WEIGHTS>  [ 1 ]
<MEANS_INVVARS>  [
  0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ]
<INV_VARS>  [
  1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 ]
</DiagGMM> "
done
} > $mdl_file
