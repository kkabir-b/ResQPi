import styles from "./StatusBadge.module.css";

export default function StatusBadge({ emergency }) {
    return (
        <div
            className={`${styles.badge} ${
                emergency ? styles.danger : styles.safe
            }`}
        >
            <span className={styles.dot}></span>

            {emergency ? "Emergency" : "Safe"}
        </div>
    );
}