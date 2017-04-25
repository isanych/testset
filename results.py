from __future__ import print_function
import sys


def same_or_none(old, new):
    if old and old != new:
        raise ValueError(str(old) + " != " + str(new))
    return new


def smaller_non_zero(old, new):
    return new if new > 0 and (old is None or old > new) else old


def str_or_empty(s):
    return "" if s is None else str(s)


class Mem:
    def __init__(self, cnt=None, size=None):
        self.cnt = cnt
        self.size = size

    def same_or_none(self, cnt, size):
        self.cnt = same_or_none(self.cnt, cnt)
        self.size = same_or_none(self.size, size)

    def __str__(self):
        return str_or_empty(self.cnt) + "," + str_or_empty(self.size)


class Test:
    def __init__(self, name):
        self.name = name
        self.number_of_runs = 0
        self.alloc = Mem()
        self.free = Mem()
        self.grow = Mem()
        self.shrink = Mem()
        self.population = None
        self.population_hit = None
        self.hit = None
        self.miss = None
        self.working_set = None

    def __str__(self):
        l = list()
        l.append(self.name)
        l.append(str_or_empty(self.population))
        l.append(str_or_empty(self.population_hit))
        l.append(str_or_empty(self.hit))
        l.append(str_or_empty(self.miss))
        l.append(str_or_empty(self.working_set))
        l.append("")
        l.append(str(self.alloc))
        l.append(str(self.free))
        l.append(str(self.grow))
        l.append(str(self.shrink))
        return ",".join(l)


tests = dict()
cnt = None
data_size = None
is_pool = False

with open(sys.argv[1]) as f:
    for line in f:
        s = line.split()
        if not s:
            continue
        if s[0] == "Testing:":
            is_wrap_alloc = "wrap_alloc" in s
            is_pool = "pool" in s
            name = s[1] + (" pool" if is_pool else "")
            cnt = same_or_none(cnt, int(s[7]))
            if name in tests:
                test = tests[name]
            else:
                test = Test(name)
                tests[name] = test
            test.number_of_runs += 1
            continue
        if line.startswith("Data size: "):
            data_size = same_or_none(data_size, int(s[2]))
            continue
        if s[0] == "Exit":
            if s[3] != "0":
                raise ValueError("Exit code " + s[3])
            continue
        if s[0] == "population,":
            test.population = smaller_non_zero(test.population, float(s[2]))
            continue
        if s[0] == "population":
            test.population_hit = smaller_non_zero(test.population_hit, float(s[3]))
            continue
        if s[0] == "hit,":
            test.hit = smaller_non_zero(test.hit, float(s[2]))
            continue
        if s[0] == "miss,":
            test.miss = smaller_non_zero(test.miss, float(s[2]))
            continue
        if s[0] == "Working":
            test.working_set = smaller_non_zero(test.working_set, int(s[3]))
            continue
        if is_pool:
            if s[0] == "New:":
                test.alloc.same_or_none(int(s[1]), int(s[3]))
        else:
            if s[0] == "Alloc:":
                test.alloc.same_or_none(int(s[1]), int(s[3]))
            elif s[0] == "Free:":
                test.free.same_or_none(int(s[1]), int(s[3]))
            elif s[0] == "Grow:":
                test.grow.same_or_none(int(s[1]), int(s[3]))
            elif s[0] == "Shrink:":
                test.shrink.same_or_none(int(s[1]), int(s[3]))
print(str(cnt) + "," + str(data_size))
for test in tests.values():
    print(test)