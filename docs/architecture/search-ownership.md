# Search ownership

`SearchContext::create(resource)` owns the transposition table, move workspaces
and context-local heuristics through RAII. Construction failure releases every
partial allocation and returns `Error`. Context is noncopyable; its moves allocate
nothing. The selected resource outlives the context, including moved destinations.
One synchronous search may run per context; separate contexts are independent.

`SearchRequest::create` copies the position and all historical hash inputs before
returning. An input span is used only during that call. The request survives Session
mutation/destruction and snapshot destruction. Limits are explicit budget/depth
values; search has no Session or configuration policy dependency. The nonthrowing
clock context and injected resource outlive the request. Desktop clocks carry no
borrowed context. A stop token owns its cancellation state.

`SearchContext::search(request)` performs no allocation after preparation and
returns a result or an allocation-free Error. It clears synchronous views and
clock pointers before returning, including cancellation and other errors. Budget
exhaustion returns the retained best legal move; cancellation returns Cancelled.
Integer evaluation, traversal, pruning and ordering retain frozen outputs.

The desktop prepares the request, shared task, thread and connection before
publishing a running task. A single QThread captures owned task data. Cancellation
requests stop directly; shutdown cancels and joins before releasing storage.
Queued completions require matching thread/task identities, then the controller
checks page, gameId, revision, generation, cancellation and closing. Failed search
starts or workspace errors suppress retry until a command changes the state.
An injected desktop search resource outlives the controller and every task; use a
resource safe for its worker thread and synchronize any external access to it.
