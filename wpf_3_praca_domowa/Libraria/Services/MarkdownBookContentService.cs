using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Libraria.Services
{
    public class MarkdownBookContentService
    {
        private const int TargetPageLength = 3500;

        public IReadOnlyList<string> LoadPages(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath) || !File.Exists(filePath))
            {
                return Array.Empty<string>();
            }

            var markdown = File.ReadAllText(filePath);
            return SplitMarkdown(markdown);
        }

        public int CountPages(string filePath)
        {
            return LoadPages(filePath).Count;
        }

        public IReadOnlyList<string> SplitMarkdown(string markdown)
        {
            if (string.IsNullOrWhiteSpace(markdown))
            {
                return Array.Empty<string>();
            }

            var normalized = (markdown ?? string.Empty).Replace("\r\n", "\n");
            var headingPages = SplitByHeadings(normalized);

            if (headingPages.Count > 1)
            {
                return headingPages;
            }

            return SplitByLength(normalized);
        }

        private static List<string> SplitByHeadings(string markdown)
        {
            var pages = new List<string>();
            var current = new List<string>();

            foreach (var line in markdown.Split('\n'))
            {
                if (line.StartsWith("# ", StringComparison.Ordinal) && current.Count > 0)
                {
                    pages.Add(string.Join(Environment.NewLine, current).Trim());
                    current.Clear();
                }

                current.Add(line);
            }

            if (current.Count > 0)
            {
                pages.Add(string.Join(Environment.NewLine, current).Trim());
            }

            return pages.Where(page => !string.IsNullOrWhiteSpace(page)).ToList();
        }

        private static List<string> SplitByLength(string markdown)
        {
            var pages = new List<string>();
            var paragraphs = markdown.Split(new[] { "\n\n" }, StringSplitOptions.None);
            var current = new List<string>();
            var currentLength = 0;

            foreach (var paragraph in paragraphs)
            {
                if (currentLength > 0 && currentLength + paragraph.Length > TargetPageLength)
                {
                    pages.Add(string.Join(Environment.NewLine + Environment.NewLine, current).Trim());
                    current.Clear();
                    currentLength = 0;
                }

                current.Add(paragraph);
                currentLength += paragraph.Length;
            }

            if (current.Count > 0)
            {
                pages.Add(string.Join(Environment.NewLine + Environment.NewLine, current).Trim());
            }

            return pages.Where(page => !string.IsNullOrWhiteSpace(page)).ToList();
        }
    }
}
