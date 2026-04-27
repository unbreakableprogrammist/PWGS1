using System.Windows;
using System.Collections.ObjectModel;
// fajne notatki :
// notion.so/xoxliwia/WPF-34cf38f0d04d807fadefd45d878a260b?source=copy_link&fbclid=IwY2xjawRcTBxleHRuA2FlbQIxMQBzcnRjBmFwcF9pZAEwAAEePlGKdtxuhLfLT23DQ2FrO4s7m774B7MdWT8UUdeXMqWmawTgy5vK8Jf3sc8_aem_Kzzxpc54PjmUcifoi2pv8Q

namespace tutorial_wpf
{
    /// <summary>
    /// Główne okno aplikacji.
    /// Przechowuje listę kontaktów i obsługuje akcje z menu.
    /// </summary>
    public partial class MainWindow : Window
    {
        // Kolekcja danych, do której podpięty jest interfejs.
        // ObservableCollection automatycznie odświeża UI po Add/Clear.
        public ObservableCollection<Contact> Contacts { get; set; }

        public MainWindow()
        {
            // Tworzy elementy graficzne z MainWindow.xaml.
            InitializeComponent();

            // Inicjalizacja pustej listy kontaktów.
            Contacts = new ObservableCollection<Contact>();

            // Ustawienie źródła bindowania dla okna.
            // Dzięki temu {Binding} w XAML widzi kolekcję Contacts.
            DataContext = Contacts;
        }

        // Obsługa menu: Contacts -> Add contact.
        // Otwiera okno modalne i po zatwierdzeniu dodaje nowy kontakt do listy.
        private void MenuItem_AddContact(object sender, RoutedEventArgs e)
        {
            Opacity = 0.5; // Wyszarzamy główne okno na czas dodawania

            // Tworzymy osobne okno do wpisania danych kontaktu.
            var addContactWindow = new AddContactWindow();

            // ShowDialog() sprawia, że nowe okno blokuje główne okno.
            // Jeśli okno zwróci "true" (czyli użytkownik kliknie "Add contact")
            if (addContactWindow.ShowDialog().Value)
            {
                // Dodajemy nowy kontakt z okienka do naszej głównej listy
                Contacts.Add(addContactWindow.NewContact);
            }

            Opacity = 1; // Zdejmujemy wyszarzenie głównego okna
        }

        // Obsługa menu: Contacts -> Clear contacts.
        // Usuwa wszystkie kontakty z kolekcji.
        private void MenuItem_ClearContacts(object sender, RoutedEventArgs e)
        {
            Contacts.Clear();
        }

        // Obsługa menu: Contacts -> About.
        // Pokazuje proste okno z informacją o aplikacji.
        private void MenuItem_About(object sender, RoutedEventArgs e)
        {
            MessageBox.Show("This is simple contact manager.", "Contact Manager", MessageBoxButton.OK, MessageBoxImage.Information);
        }

        // Obsługa menu: File -> Exit.
        // Zamknięcie głównego okna kończy działanie aplikacji.
        private void MenuItem_Exit(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
}