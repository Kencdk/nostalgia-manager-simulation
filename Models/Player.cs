using System;

namespace NostalgiaManager.Models
{
    public class Player
    {
        public string Name { get; set; }
        public DateTime Birthday { get; set; }

        // Helper method to calculate age relative to a fixed date (July 1, 1997)
        public int CalculateAge(DateTime referenceDate = new DateTime(1997, 7, 1))
        {
            var today = this.Birthday;
            int age = referenceDate.Year - today.Year;

            // Adjust if the birthday hasn't passed yet in the current year relative to the fixed date
            if (today > new DateTime(referenceDate.Year, today.Month, today.Day))
            {
                age--;
            }
            return age;
        }
    }
}