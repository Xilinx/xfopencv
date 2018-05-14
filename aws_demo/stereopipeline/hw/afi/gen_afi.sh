#!/bin/bash
echo aws s3 rm --recursive s3://xfsp
aws s3 rm --recursive s3://xfsp

echo aws s3 rb s3://xfsp
aws s3 rb s3://xfsp


echo aws s3 mb s3://xfsp
aws s3 mb s3://xfsp

aws s3 mb s3://xfsp/dcp
touch FILES_GO_HERE.txt
aws s3 cp FILES_GO_HERE.txt s3://xfsp/dcp/


aws s3 mb s3://xfsp/log
touch LOGS_FILES_GO_HERE.txt
aws s3 cp LOGS_FILES_GO_HERE.txt s3://xfsp/log/  

aws s3 ls --recursive s3://xfsp

rm -f FILES_GO_HERE.txt
rm -f LOGS_FILES_GO_HERE.txt

$SDACCEL_DIR/tools/create_sdaccel_afi.sh -xclbin=xf_stereo_pipeline.xclbin -s3_bucket=xfsp -s3_dcp_key=dcp -s3_logs_key=log

cat *afi_id*

echo "use following command to check afi ready"
echo "aws ec2 describe-fpga-images --fpga-image-id <afi id>"
