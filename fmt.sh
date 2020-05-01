perl -i -pe 's{assert\((.*?), (.*), ".*"\);}{($a,$b)=($1,$2); (($c=$2) =~ s/([\\"])/\\\1/g); "assert($a, $b, \"$c\");"}ge' tests/tests.c
