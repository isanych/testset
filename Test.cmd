@echo %DATE% %TIME% Start ============================================================
@set c=TimeMem TestSet 20000000 pop_hit 1 hit 1 miss 1
%c% boost::unordered_set
%c% spp::sparse_hash_set
%c% unordered
%c% btree
%c% sparse
%c% dense
%c% closed
%c% forward
%c% huge_forward
%c% huge_linked
%c% set
@set c=TimeMem TestSetAlloc 20000000 pop_hit 1 hit 1 miss 1
%c% boost::unordered_set
%c% spp::sparse_hash_set
%c% unordered
%c% btree
%c% sparse
%c% dense
%c% closed
%c% forward
%c% huge_forward
%c% huge_linked
%c% set
@set c=TimeMem TestSetPool 20000000 pop_hit 1 hit 1 miss 1
%c% boost::unordered_set
%c% spp::sparse_hash_set
%c% unordered
%c% set
:finish
@echo %DATE% %TIME% Stop =============================================================
