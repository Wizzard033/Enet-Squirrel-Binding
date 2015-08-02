Here is an example of how to load it up (depends on http://scrat.sourceforge.net/)

    // Make sure to perform first time setup only once
    static bool isFirstRun = true;
    if (isFirstRun) {
        // Initialize ENet
        if (enet_initialize() != 0) {
            // An error occurred while initializing ENet
        }

        // Indicate that we've ran this first time setup
        isFirstRun = false;
    }

    // Initialize and register the enet namespace in the VM
    Sqrat::Table enetNamespace(v);
    Sqrat::RootTable(v).Bind(_SC("enet"), enetNamespace);

    // Initialize and register the ENet Squirrel library in the VM
    RegisterEnetLib(v, enetNamespace);


** This binding will not be maintained by its developer, but merge requests will be reviewed **
