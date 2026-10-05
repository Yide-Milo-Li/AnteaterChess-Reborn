# Session ownership

`ac::Session::create` returns `Result<Session>`. Session cannot be copied; moving
transfers the complete owner and its memory resource without allocating. A moved
Session may be destroyed or assigned; its commands reject use and `valid()` is false.

`state()` returns a value containing position, clocks, configuration and revision.
It allocates no storage and contains no borrowed history pointers. Desktop clock
polling uses this value. `snapshot(resource)` returns an owning, movable
`SessionSnapshot` containing all active moves and position hashes. The snapshot
remains valid after Session commands, restart, movement and destruction. Moving
it adopts its originating resource, including assignment between resources.

An injected memory resource must outlive every allocation made through it:
Session, its snapshots, and any destination owner after moving. A separately
supplied snapshot resource need only outlive that snapshot. Resources report
allocation failure by throwing `std::bad_alloc`. Construction and snapshot
factories translate it into allocation-free `Error` metadata and release partial
allocations. Clock callbacks must not throw; their context outlives Session.

Commands retain the baseline timeout policy: submit and undo tick first. A
timeout can advance the turn and revision even when the command returns stale.
After that tick, move preparation finishes before position/history publication.
Session keeps the full fixed capacity available throughout a game.

Logs belong to the desktop runtime. Accepted commands keep their success result
when snapshot projection or atomic log replacement fails; the controller displays
a diagnostic. A normal clock tick takes no snapshot and writes no log. Narrow
desktop notifications and owned search requests are subsequent migration stages.
