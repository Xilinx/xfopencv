#!/bin/bash
echo aws s3 rm --recursive s3://xfg
aws s3 rm --recursive s3://xfg

echo aws s3 rb s3://xfg
aws s3 rb s3://xfg


echo aws s3 mb s3://xfg
aws s3 mb s3://xfg

aws s3 mb s3://xfg/dcp
touch FILES_GO_HERE.txt
aws s3 cp FILES_GO_HERE.txt s3://xfg/dcp/


aws s3 mb s3://xfg/log
touch LOGS_FILES_GO_HERE.txt
aws s3 cp LOGS_FILES_GO_HERE.txt s3://xfg/log/  

aws s3 ls --recursive s3://xfg

rm -f FILES_GO_HERE.txt
rm -f LOGS_FILES_GO_HERE.txt

$SDACCEL_DIR/tools/create_sdaccel_afi.sh -xclbin=xf_gaussian_filter.xclbin -s3_bucket=xfg -s3_dcp_key=dcp -s3_logs_key=log

cat *afi_id*

echo "use following command to check afi ready"
echo "aws ec2 describe-fpga-images --fpga-image-id <afi id>"
