../build/run "../conf/example_comp.json" \
| grep -e "computeFingerprint" -e "compression" -e "decompression" \
       -e "lookup" -e "dedup" -e "update_index" -e "io_ssd" -e "io_hdd" \
| awk '{print $5}'