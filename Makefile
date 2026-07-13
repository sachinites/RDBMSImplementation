# Top-level RDBMS build entry point (delegates to SqlParser/)

.PHONY: all clean lib dbms test bptree

all clean lib dbms test bptree:
	$(MAKE) -C SqlParser $@
