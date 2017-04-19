@echo %DATE% %TIME% Start ============================================================
@set c=TimeMem TestSet 20000000 hit 1 miss 1
%c% unordered
%c% btree
%c% sparse
%c% dense
%c% closed
%c% forward
%c% huge_forward
%c% huge_linked
%c% set
@set c=TimeMem TestSetAlloc 20000000 hit 1 miss 1
%c% unordered
%c% btree
%c% sparse
%c% dense
%c% closed
%c% forward
%c% huge_forward
%c% huge_linked
%c% set
@set c=TimeMem TestSetPool 20000000 hit 1 miss 1
%c% unordered
%c% btree
%c% sparse
%c% dense
%c% closed
%c% forward
%c% huge_forward
%c% huge_linked
%c% set
:finish
@echo %DATE% %TIME% Stop =============================================================
