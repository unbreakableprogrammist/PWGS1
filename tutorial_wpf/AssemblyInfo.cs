using System.Windows;

// ThemeInfo mówi WPF gdzie szukać ResourceDictionary,
// gdy zasób (np. styl/szablon) nie zostanie znaleziony lokalnie.
[assembly: ThemeInfo(
    ResourceDictionaryLocation.None,            //where theme specific resource dictionaries are located
                                                //(used if a resource is not found in the page,
                                                // or application resource dictionaries)
    ResourceDictionaryLocation.SourceAssembly   //where the generic resource dictionary is located
                                                //(used if a resource is not found in the page,
                                                // app, or any theme specific resource dictionaries)
)]
