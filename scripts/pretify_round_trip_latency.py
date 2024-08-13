import pandas
import sys
print(pandas.read_csv(sys.stdin).drop(columns=['75_msg_ns', '90_msg_ns', 'producer_n','consumer_n', '99_msg_ns']).sort_values(by=['50_msg_ns','name']).to_markdown(index=False))