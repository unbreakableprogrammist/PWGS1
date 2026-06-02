using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace Libraria.Models
{
    public enum BookStatus
    {
        Unread,
        InProgress,
        Read
    }

    public class Book : INotifyPropertyChanged
    {
        private Guid _id = Guid.NewGuid();
        private string _title = string.Empty;
        private string _author = string.Empty;
        private string _description = string.Empty;
        private string _genre = string.Empty;
        private ObservableCollection<string> _tags = new();
        private int _rating;
        private BookStatus _status = BookStatus.Unread;
        private byte[] _coverImage = Array.Empty<byte>();
        private string _coverImagePath = string.Empty;
        private string _filePath = string.Empty;
        private int _currentPage = 1;
        private int _pageCount;
        private DateTime _createdAt = DateTime.Now;
        private DateTime _updatedAt = DateTime.Now;

        public event PropertyChangedEventHandler? PropertyChanged;

        public Guid Id
        {
            get => _id;
            set => SetProperty(ref _id, value);
        }

        public string Title
        {
            get => _title;
            set => SetProperty(ref _title, value ?? string.Empty);
        }

        public string Author
        {
            get => _author;
            set => SetProperty(ref _author, value ?? string.Empty);
        }

        public string Description
        {
            get => _description;
            set => SetProperty(ref _description, value ?? string.Empty);
        }

        public string Genre
        {
            get => _genre;
            set => SetProperty(ref _genre, value ?? string.Empty);
        }

        public ObservableCollection<string> Tags
        {
            get => _tags;
            set => SetProperty(ref _tags, value ?? new ObservableCollection<string>());
        }

        public int Rating
        {
            get => _rating;
            set => SetProperty(ref _rating, Math.Clamp(value, 0, 5));
        }

        public BookStatus Status
        {
            get => _status;
            set => SetProperty(ref _status, value);
        }

        public byte[] CoverImage
        {
            get => _coverImage;
            set => SetProperty(ref _coverImage, value ?? Array.Empty<byte>());
        }

        public string CoverImagePath
        {
            get => _coverImagePath;
            set => SetProperty(ref _coverImagePath, value ?? string.Empty);
        }

        public string FilePath
        {
            get => _filePath;
            set => SetProperty(ref _filePath, value ?? string.Empty);
        }

        public int CurrentPage
        {
            get => _currentPage;
            set => SetProperty(ref _currentPage, Math.Max(1, value));
        }

        public int PageCount
        {
            get => _pageCount;
            set => SetProperty(ref _pageCount, Math.Max(0, value));
        }

        public DateTime CreatedAt
        {
            get => _createdAt;
            set => SetProperty(ref _createdAt, value);
        }

        public DateTime UpdatedAt
        {
            get => _updatedAt;
            set => SetProperty(ref _updatedAt, value);
        }

        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        protected bool SetProperty<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
        {
            if (EqualityComparer<T>.Default.Equals(field, value))
            {
                return false;
            }

            field = value;
            OnPropertyChanged(propertyName);
            return true;
        }
    }
}
