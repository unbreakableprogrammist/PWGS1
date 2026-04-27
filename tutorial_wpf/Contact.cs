namespace tutorial_wpf
{
    // Model danych pojedynczego kontaktu.
    // Obiekty tej klasy są przechowywane w ObservableCollection<Contact> w MainWindow.
    public class Contact
    {
        // Podstawowe dane osobowe kontaktu.
        public string? Name { get; set; }
        public string? Surname { get; set; }

        // Dane kontaktowe.
        public string? Email { get; set; }
        public string? Phone { get; set; }

        // Płeć kontaktu używana m.in. do wyboru avatara w liście.
        public Gender Gender { get; set; }
    }

    // Enum z możliwymi wartościami płci.
    // Jest używany przez ComboBox w AddContactWindow.
    public enum Gender
    {
        Male,
        Female
    }
}