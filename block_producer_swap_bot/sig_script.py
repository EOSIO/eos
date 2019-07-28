# from eosiopy.sign import sign
#
#
# if __name__ == '__main__':
#     sig = sign('5KLGj1HGRWbk5xNmoKfrcrQHXvcVJBPdAckoiJgFftXSJjLPp7b', bytes.fromhex('374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb'))
#     print(sig)
from _sha256 import sha256
import codecs
from eosiopy.sign import sign


if __name__ == '__main__':
    swap_id_str = 'receiver' + '*' 'receiver' + '*' + 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC' + '*' + \
                  'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
    swap_id_str = swap_id_str.encode()

    print(swap_id_str)
    sig = sign('5JGzrjkwagH3MXvMvBdfWPzUrPEwSqrtNCXKdzzbtzjkhcbSjLc', swap_id_str)
    print(sig)
    # print(sha256(bytes.fromhex('b194c53ac3df12704faf4247059d903ef5e9bd4f80dc393fba248114a86a7dd4')).hexdigest())
    # print(sha256(b'7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC').hexdigest())
