#!/bin/bash
pwd=`pwd`
down()
{
	$pwd/bin/request https://gz.blockchair.com/bitcoin/blocks/blockchair_bitcoin_blocks_20090103.tsv.gz
}
down
