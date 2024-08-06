import pandas
import sys
print(pandas.read_csv(sys.stdin).drop(columns=['75_msg_sec', '90_msg_sec', 'producer_n','consumer_n', '99_msg_sec']).sort_values(by=['50_msg_sec','name'],ascending=False).to_markdown(index=False))