using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace tutorial_wpf
{
    /// <summary>
    /// Okno dialogowe do dodawania nowego kontaktu.
    ///
    /// Działa w trybie modalnym (ShowDialog), więc użytkownik
    /// najpierw kończy operację tutaj, a dopiero potem wraca do MainWindow.
    /// </summary>
    public partial class AddContactWindow : Window
    {
        // Obiekt Contact budowany na podstawie danych wpisanych w formularzu.
        // MainWindow odczyta tę właściwość po zamknięciu dialogu z wynikiem true.
        public Contact NewContact { get; private set; }

        public AddContactWindow()
        {
            InitializeComponent();

            // Tworzymy pusty model kontaktu i ustawiamy go jako DataContext,
            // żeby bindingi z XAML (Name/Surname/Email/Phone/Gender) działały dwukierunkowo.
            NewContact = new Contact();
            DataContext = NewContact;
        }

        // Kliknięcie "Add contact":
        // DialogResult=true informuje okno nadrzędne, że dane zostały zatwierdzone.
        private void AddContact(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
            Close();
        }

        // Kliknięcie "Cancel":
        // DialogResult=false oznacza rezygnację z dodawania kontaktu.
        private void Cancel(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
            Close();
        }

        // Miejsce na dodatkową logikę po zmianie wyboru płci (aktualnie nieużywane).
        private void ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

        }

    }
}
