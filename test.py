import sys
import random
import string

def random_str(str_len):
    clist = []
    chars = string.ascii_uppercase + string.ascii_lowercase + string.digits
    for _ in range(str_len):
        clist.append(random.choice(chars))
    return ''.join(clist)


def generate_test_data(num_rows, from_id):
    rows = []
    for i in range(num_rows):
        rid = from_id + i
        user = random_str(5)
        email = '%s@gmail.com' % user
        rows.append((rid, user, email))
    return rows


if __name__ == '__main__':
    lines = int(sys.argv[1])
    from_id = int(sys.argv[2])
    for rid, user, email in generate_test_data(lines, from_id):
        print(rid, user, email)
